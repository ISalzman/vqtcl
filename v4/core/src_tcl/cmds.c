/*
 * cmds.c - Implementation of Tcl-specific operators.
 */

#include "ext_tcl.h"

ItemTypes DefCmd_OO (Item args[]) {
    int argc, c, r, cols, rows;
    View_p meta, result;
    Object_p obj, *argv;
    Column column;
    struct Buffer buffer;
    
    if (MdefCmd_O(args) != IT_view)
        return IT_error;
    if (args[0].v == NULL)
        return IT_view;
    meta = args[0].v;
        
    if (Tcl_ListObjGetElements(Interp(), args[1].o, &argc, &argv) != TCL_OK)
        return IT_unknown;
        
    cols = ViewSize(meta);
    if (cols == 0) {
        if (argc != 0) {
            args->e = EC_cizwv;
            return IT_error;
        }
        
        args->v = NoColumnView(0);
        return IT_view;
    }
    
    if (argc % cols != 0) {
        args->e = EC_nmcw;
        return IT_error;
    }

    result = NewView(meta);
    rows = argc / cols;
    
    for (c = 0; c < cols; ++c) {
        InitBuffer(&buffer);

        for (r = 0; r < rows; ++r)
            ADD_PTR_TO_BUF(buffer, argv[c + cols * r]);

        obj = BufferAsTclList(&buffer);
        column = CoerceColumn(ViewColType(result, c), obj);
        DecObjRef(obj);
        
        if (column.seq == NULL)
            return IT_unknown;
            
        SetViewCols(result, c, 1, column);
    }
    
    args->v = result;
    return IT_view;
}

static Object_p GetViewRows (View_p view, int row, int rows, int tags) {
    int r, c, cols;
    View_p meta;
    Item item;
    struct Buffer buf;
    
    meta = V_Meta(view);
    cols = ViewSize(meta);
    InitBuffer(&buf);
    
    for (r = 0; r < rows; ++r)
        for (c = 0; c < cols; ++c) {
            if (tags) {
                item.c = ViewCol(meta, 0);
                ADD_PTR_TO_BUF(buf, ItemAsObj(GetItem(c, &item), &item));
            }
            item.c = ViewCol(view, c);
            ADD_PTR_TO_BUF(buf, ItemAsObj(GetItem(r + row, &item), &item));
        }
        
    return BufferAsTclList(&buf);
}

static int ColumnNumber(Object_p obj, View_p meta) {
    int col;
    const char *name = Tcl_GetString(obj);
    
    switch (*name) {

        case '#': return -2;
        case '*': return -1;
        case '-': case '0': case '1': case '2': case '3': case '4':
                  case '5': case '6': case '7': case '8': case '9':
            if (Tcl_GetIntFromObj(Interp(), obj, &col) != TCL_OK)
                return -3;

            if (col < 0)
                col += ViewSize(meta);
                
            if (col < 0 || col >= ViewSize(meta))
                return -4;
            break;

        default:
            col = StringLookup(name, ViewCol(meta, MC_name));
            if (col < 0)
                return -3;
    }

    return col;
}

ItemTypes GetCmd_VX (Item args[]) {
    View_p view;
    int i, objc, r, row, col, rows;
    const Object_p *objv;
    ItemTypes currtype;

    view = args[0].v;
    currtype = IT_view;
    
    objv = (const void*) args[1].u.ptr;
    objc = args[1].u.len;

    if (objc == 0) {
        args->o = GetViewRows(view, 0, ViewSize(view), 0);
        return IT_object;
    }
    
    for (i = 0; i < objc; ++i) {
        if (currtype == IT_view)
            view = args[0].v;
        else /* TODO: make sure the object does not leak if newly created */
            view = ObjAsView(ItemAsObj(currtype, args));
        
        if (view == NULL)
            return IT_unknown;

        rows = ViewSize(view);

        if (Tcl_GetIntFromObj(0, objv[i], &row) != TCL_OK)
            switch (*Tcl_GetString(objv[i])) {
                case '@':   args->v = V_Meta(view);
                            currtype = IT_view;
                            continue;
                case '#':   args->i = rows;
                            currtype = IT_int;
                            continue;
                case '*':   row = -1;
                            break;
                default:    Tcl_GetIntFromObj(Interp(), objv[i], &row);
                            return IT_unknown;
            }
        else if (row < 0) {
            row += rows;
            if (row < 0) {
                args->e = EC_rioor;
                return IT_error;
            }
        }

        if (++i >= objc) {
            if (row >= 0)
                args->o = GetViewRows(view, row, 1, 1); /* one tagged row */
            else {
                struct Buffer buf;
                InitBuffer(&buf);
                for (r = 0; r < rows; ++r)
                    ADD_PTR_TO_BUF(buf, GetViewRows(view, r, 1, 0));
                args->o = BufferAsTclList(&buf);
            }
            return IT_object;
        }

        col = ColumnNumber(objv[i], V_Meta(view));

        if (row >= 0)
            switch (col) {
                default:    args->c = ViewCol(view, col);
                            currtype = GetItem(row, args);
                            break;
                case -1:    args->o = GetViewRows(view, row, 1, 0);
                            currtype = IT_object;
                            break;
                case -2:    args->i = row;
                            currtype = IT_int;
                            break;
                case -3:    return IT_unknown;
                case -4:    args->e = EC_cioor;
                            return IT_error;
            }
        else
            switch (col) {
                default:    args->c = ViewCol(view, col);
                            currtype = IT_column;
                            break;
                case -1:    args->o = GetViewRows(view, 0, rows, 0);
                            currtype = IT_object;
                            break;
                case -2:    args->c = NewIotaColumn(rows);
                            currtype = IT_column;
                            break;
                case -3:    return IT_unknown;
                case -4:    args->e = EC_cioor;
                            return IT_error;
            }
    }
    
    return currtype;
}

ItemTypes MutInfoCmd_V (Item_p a) {
    int i;
    Seq_p mods, *seqs;
    Object_p result;
    
    if (!IsMutable(a[0].v))
        return IT_unknown;
        
    mods = MutPrepare(a[0].v);
    seqs = (void*) (mods + 1);
    
    result = Tcl_NewListObj(0, NULL);

    for (i = 0; i < MP_delmap; ++i)
        Tcl_ListObjAppendElement(NULL, result, ViewAsObj(seqs[i]));
    for (i = MP_delmap; i < MP_limit; ++i)
        Tcl_ListObjAppendElement(NULL, result, ColumnAsObj(SeqAsCol(seqs[i])));
    
    a->o = result;
    return IT_object;
}

ItemTypes RefsCmd_O (Item args[]) {
    args->i = args[0].o->refCount;
    return IT_int;
}

ItemTypes RenameCmd_VO (Item args[]) {
    int i, c, objc;
    View_p meta, view, newmeta, newview;
    Object_p *objv;
    Item item;

    if (Tcl_ListObjGetElements(Interp(), args[1].o, &objc, &objv) != TCL_OK)
        return IT_unknown;
        
    if (objc % 2) {
        args->e = EC_rambe;
        return IT_error;
    }

    view = args[0].v;
    meta = V_Meta(view);
    newmeta = meta;
    
    for (i = 0; i < objc; i += 2) {
        c = ColumnByName(meta, objv[i]);
        if (c < 0)
            return IT_unknown;
        item.s = Tcl_GetString(objv[i+1]);
        newmeta = ViewSet(newmeta, c, MC_name, &item);
    }
    
    newview = NewView(newmeta);
    
    for (c = 0; c < ViewWidth(view); ++c)
        SetViewCols(newview, c, 1, ViewCol(view, c));

    args->v = newview;
    return IT_view;
}

ItemTypes ToCmd_OO (Item args[]) {
    Object_p result;

    result = Tcl_ObjSetVar2(Interp(), args[1].o, 0, args[0].o,
                                                    TCL_LEAVE_ERR_MSG);
    if (result == NULL)
        return IT_unknown;

    args->o = result;
    return IT_object;
}

ItemTypes TypeCmd_O (Item args[]) {
    args->s = args[0].o->typePtr != NULL ? args[0].o->typePtr->name : "";
    return IT_string;
}
