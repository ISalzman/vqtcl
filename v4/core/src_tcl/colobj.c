/*
 * colobj.c - Implementation of column objects in Tcl.
 */

#include "ext_tcl.h"

static const char *ErrorMessage(ErrorCodes code) {
	switch (code) {
		case EC_cioor: return "column index out of range";
		case EC_rioor: return "row index out of range";
		case EC_cizwv: return "cannot insert in zero-width view";
		case EC_nmcw:	 return "item count not a multiple of column width";
		case EC_rambe: return "rename arg count must be even";
		case EC_nalor: return "nead at least one row";
		case EC_wnoa:	 return "wrong number of arguments";
	}
	return "?";
}

Object_p ItemAsObj (ItemTypes type, Item_p item) {
	switch (type) {

		case IT_int:
			return Tcl_NewIntObj(item->i);

		case IT_wide:
			return Tcl_NewWideIntObj(item->w);

		case IT_float:
			return Tcl_NewDoubleObj(item->f);

		case IT_double:
			return Tcl_NewDoubleObj(item->d);

		case IT_string:
			return Tcl_NewStringObj(item->s, -1);
			
		case IT_bytes:
			return Tcl_NewByteArrayObj(item->u.ptr, item->u.len);

		case IT_object:
			return item->o;

		case IT_column:
			return ColumnAsObj(item->c);

		case IT_view:
			if (item->v == NULL) {
				Tcl_SetResult(Interp(), "invalid view", TCL_STATIC);
				return NULL;
			}
			return ViewAsObj(item->v);

		case IT_error:
			Tcl_SetResult(Interp(), (char*) ErrorMessage(item->e), TCL_STATIC);
			return NULL;

		default:
			Assert(0);
			return NULL;
	}
}

int ColumnByName (View_p meta, Object_p obj) {
	int colnum;
	const char *str = Tcl_GetString(obj);
	
	switch (*str) {

		case '#':
			return -2;

		case '*':
			return -3;

		case '-': case '0': case '1': case '2': case '3': case '4':
				  case '5': case '6': case '7': case '8': case '9':
			if (Tcl_GetIntFromObj(NULL, obj, &colnum) != TCL_OK)
				return -4;
			if (colnum < 0)
				colnum += ViewSize(meta);
			if (colnum < 0 || colnum >= ViewSize(meta))
				return -5;
			return colnum;
			
		default:
			return StringLookup(str, ViewCol(meta, MC_name));
	}
}

int ObjToItem (ItemTypes type, Item_p item) {
	switch (type) {
		
		case IT_int:
			return Tcl_GetIntFromObj(Interp(), item->o, &item->i) == TCL_OK;

		case IT_wide:
			return Tcl_GetWideIntFromObj(Interp(), item->o,
											(Tcl_WideInt*) &item->w) == TCL_OK;

		case IT_float:
			if (Tcl_GetDoubleFromObj(Interp(), item->o, &item->d) != TCL_OK)
				return 0;
			item->f = (float) item->d;
			break;

		case IT_double:
			return Tcl_GetDoubleFromObj(Interp(), item->o, &item->d) == TCL_OK;

		case IT_string:
			item->s = Tcl_GetString(item->o);
			break;

		case IT_bytes:
			item->u.ptr = Tcl_GetByteArrayFromObj(item->o, &item->u.len);
			break;

		case IT_object:
			break;

		case IT_column:
			item->c = ObjAsColumn(item->o);
			return item->c.seq != NULL && item->c.seq->count >= 0;

		case IT_view:
			item->v = ObjAsView(item->o);
			return item->v != NULL;

		default:
			Assert(0);
			return 0;
	}
	
	return 1;
}

#define OBJ_TO_COLUMN_P(o) ((Column*) &(o)->internalRep.twoPtrValue)

static void FreeColIntRep (Tcl_Obj *obj) {
	PUSH_KEEP_REFS
	DecRefCount(OBJ_TO_COLUMN_P(obj)->seq);
	POP_KEEP_REFS
}

static void DupColIntRep (Tcl_Obj *src, Tcl_Obj *dup) {
	Column_p column = OBJ_TO_COLUMN_P(src);
	IncRefCount(column->seq);
	*OBJ_TO_COLUMN_P(dup) = *column;
	dup->typePtr = &f_colObjType;
}

static void UpdateColStrRep (Tcl_Obj *obj) {
	int r;
	Object_p list;
	Item item;
	struct Buffer buf;
	Column column;
	PUSH_KEEP_REFS
	
	InitBuffer(&buf);
	column = *OBJ_TO_COLUMN_P(obj);
	
	if (column.seq != NULL)
		for (r = 0; r < column.seq->count; ++r) {
			item.c = column;
			ADD_PTR_TO_BUF(buf, ItemAsObj(GetItem(r, &item), &item));
		}

	list = BufferAsTclList(&buf);

	/* this hack avoids creating a second copy of the string rep */
	obj->bytes = Tcl_GetStringFromObj(list, &obj->length);
	list->bytes = NULL;
	list->length = 0;
	
	DecObjRef(list);
	
	POP_KEEP_REFS
}

#define LO_list		data[0].o
#define LO_objv		data[1].p

static void ListObjCleaner (Seq_p seq) {
	DecObjRef(seq->LO_list);
}

static ItemTypes ListObjGetter (int row, Item_p item) {
	const Object_p *objv = item->c.seq->LO_objv;
	item->o = objv[row];
	return IT_object;
}

static struct SeqType ST_ListObj = {
	"listobj", ListObjGetter, 0, ListObjCleaner
};

static int SetColFromAnyRep (Tcl_Interp *interp, Tcl_Obj *obj) {
	int objc;
	Object_p dup, *objv;
	Seq_p seq;

	/* must duplicate the list because it'll be kept by ObjAsColumn */
	dup = Tcl_DuplicateObj(obj);
	
	if (Tcl_ListObjGetElements(interp, dup, &objc, &objv) != TCL_OK) {
		DecObjRef(dup);
		return TCL_ERROR;
	}

	PUSH_KEEP_REFS
	
	seq = IncRefCount(NewSequence(objc, &ST_ListObj, 0));
	seq->LO_list = IncObjRef(dup);
	seq->LO_objv = objv;

	if (obj->typePtr != NULL && obj->typePtr->freeIntRepProc != NULL)
		obj->typePtr->freeIntRepProc(obj);

	*OBJ_TO_COLUMN_P(obj) = SeqAsCol(seq);
	obj->typePtr = &f_colObjType;
	
	POP_KEEP_REFS
	return TCL_OK;
}

Tcl_ObjType f_colObjType = {
	"column", FreeColIntRep, DupColIntRep, UpdateColStrRep, SetColFromAnyRep
};

Column ObjAsColumn (Object_p obj) {
	if (Tcl_ConvertToType(Interp(), obj, &f_colObjType) != TCL_OK) {
		Column column;
		column.seq = NULL;
		column.pos = -1;
		return column;
	}
	
	return *OBJ_TO_COLUMN_P(obj);
}

Object_p ColumnAsObj (Column column) {
	Object_p result;

	result = Tcl_NewObj();
	Tcl_InvalidateStringRep(result);

	IncRefCount(column.seq);
	*OBJ_TO_COLUMN_P(result) = column;
	result->typePtr = &f_colObjType;
	return result;
}
