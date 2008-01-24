/*
 * vlerq_ext.c - Interface to the Ruby scripting language.
 */

#include <ruby.h>

#include <stdlib.h>
#include <string.h>

#include "vlerq.h"

typedef struct CmdDispatch {
  const char *name, *args;
  ItemTypes (*proc) (Item_p);
} CmdDispatch;

static CmdDispatch f_commands[] = {
#include "vlerq_ext.h"
};

static ID id_lshift;

struct Shared *GetShared (void) {
  static struct Shared f_shared;
  return &f_shared;
}

void FailedAssert (const char *msg, const char *file, int line) {
  rb_fatal("Failed assertion at %s, line %d: %s\n", file, line, msg);
}

typedef struct { VALUE view; int row; } Row, *Row_p;

static VALUE cRow;

static VALUE row_v (VALUE self) {
  Row_p p;
  Data_Get_Struct(self, Row, p);
  return p->view;
}

static VALUE row_i (VALUE self) {
  Row_p p;
  Data_Get_Struct(self, Row, p);
  return INT2NUM(p->row);
}

static void row_mark (Row_p p) {
  rb_gc_mark(p->view);
}

static VALUE AtRow (VALUE view, int row) {
  Row_p p = ALLOC(Row);
  p->view = view;
  p->row = row;
  return Data_Wrap_Struct(cRow, row_mark, 0, p);
}

ItemTypes AtRowCmd_OI (Item_p args) {
  args[0].o = AtRow(args[0].o, args[1].i);
  return IT_object;
}

static VALUE cCol;

static ItemTypes ListObjGetter (int row, Item_p item) {
  VALUE ary = (VALUE) item->c.seq->data[0].p;
  item->o = rb_ary_entry(ary, row);
  return IT_object;
}

static struct SeqType ST_ListObj = { "listobj", ListObjGetter };

static void col_mark (Column_p p) {
  if (p->seq->type == &ST_ListObj)
    rb_gc_mark((VALUE) p->seq->data[0].p);
}

static void col_free_keep (Column_p p) {
  PUSH_KEEP_REFS
  DecRefCount(p->seq);
  POP_KEEP_REFS
}

static VALUE cView;

static void view_free_keep (View_p p) {
  PUSH_KEEP_REFS
  DecRefCount(p);
  POP_KEEP_REFS
}

static VALUE view_alloc (VALUE klass) {
  return Data_Wrap_Struct(klass, 0, view_free_keep, 0);
}

View_p ObjAsMetaView (Object_p obj) {
  return NULL;
}

VALUE ItemAsObj (ItemTypes type, Item_p item) {
  VALUE result;
  
  switch (type) {
    
    case IT_int:
      result = INT2NUM(item->i);
      break;
      
    case IT_wide:
      result = LL2NUM(item->w);
      break;
      
    case IT_float:
      result = rb_float_new(item->f);
      break;
      
    case IT_double:
      result = rb_float_new(item->d);
      break;
      
    case IT_string:
      result = rb_tainted_str_new(item->s, strlen(item->s));
      break;
      
    case IT_bytes:
      result = rb_tainted_str_new((const char*) item->u.ptr, item->u.len);
      break;
      
    case IT_column: {
      Column_p p;
      result = Data_Make_Struct(cCol, Column, col_mark, col_free_keep, p);
      *p = item->c;
      IncRefCount(p->seq);
      break;
    }
    
    case IT_view:
      result = Data_Wrap_Struct(cView, 0, view_free_keep, IncRefCount(item->v));
      break;
    
    case IT_object:
      result = item->o;
      break;
      
    default:
      rb_raise (rb_eRuntimeError, "unsupported type %d", type);
  }
  
  return result;
}

ItemTypes OpenCmd_S (Item_p args) {
	args[0].v = OpenDataFile(args[0].s);
	return IT_view;
}

int ObjToItem (ItemTypes type, Item_p item) {
  switch (type) {
    
    case IT_int:
      item->i = NUM2INT(item->o);
      break;

    case IT_wide:
      item->w = NUM2LL(item->o);
      break;

    case IT_float:
      item->f = (float) NUM2DBL(item->o);
      break;

    case IT_double:
      item->d = NUM2DBL(item->o);
      break;

    case IT_string:
      item->s = STR2CSTR(item->o);
      break;

    case IT_bytes:
      item->u.ptr = (const void*) rb_str2cstr(item->o, (long*) &item->u.len);
      break;

    case IT_object:
      break;

    case IT_column:
      if (rb_class_of(item->o) != cCol) 
        return 0;
      Data_Get_Struct(item->o, Column, item->p);
      item->c = *(Column_p) item->p;
      break;

    case IT_view:
      if (TYPE(item->o) == T_FIXNUM)
        item->v = NoColumnView(NUM2INT(item->o));
      if (rb_class_of(item->o) == cView)
        Data_Get_Struct(item->o, struct Sequence, item->v);
      else 
        return 0;
      break;

    default:
      return 0;
  }
  
  return 1;
}

int ColumnByName (View_p meta, Object_p obj) {
  int colnum;

  if (TYPE(obj) == T_FIXNUM) {
    colnum = NUM2INT(obj);
    if (colnum < 0)
      colnum += ViewSize(meta);
    if (colnum < 0 || colnum >= ViewSize(meta))
      return -5;
  } else
    colnum = StringLookup(STR2CSTR(obj), ViewCol(meta, MC_name));
    
  return colnum;
}

Column ObjAsColumn (VALUE obj) {
  int objc;
  Object_p *objv;
  Sequence_p seq;

  if (rb_class_of(obj) == cCol) {
    Item item;
    item.o = obj;
    if (!ObjToItem(IT_column, &item))
      Assert(0);
    return item.c;
  }
  
  Assert(TYPE(obj) == T_ARRAY);
  objc = (int) RARRAY(obj)->len;

  seq = NewSequence(objc, &ST_ListObj, 0);
  seq->data[0].p = (void*) obj;
  return SeqAsCol(seq);
}

Object_p
NeedMutable (Object_p obj) {
  return obj;
}

static VALUE view_initialize_keep (VALUE self, VALUE arg) {
  View_p view;
  Item item;
  PUSH_KEEP_REFS
  
  item.o = arg;
  if (!ObjToItem(IT_view, &item))
    Assert(0);
    
  DATA_PTR(self) = IncRefCount(item.v);
  
  POP_KEEP_REFS
  return self;
}

#define MAX_STACK 20

static VALUE vq_dispatch_keep (int argc, VALUE *argv, VALUE self) {
  int i, index;
  VALUE result;
  Item stack [MAX_STACK];
  ItemTypes type;
  const char *args;
  
  index = NUM2INT(argv[0]);
  
  PUSH_KEEP_REFS
  
  --argc; ++argv;
  args = f_commands[index].args + 2; /* skip return type and ':' */
  
  for (i = 0; args[i] != 0; ++i) {
    Assert(i < MAX_STACK);
    if (args[i] == 'X') {
      Assert(args[i+1] == 0);
      stack[i].u.ptr = (const void*) (argv+i);
      stack[i].u.len = argc-i;
      break;
    }
    if ((args[i] == 0 && i != argc) || (args[i] != 0 && i >= argc))
      rb_raise(rb_eRuntimeError, "wrong number of arguments");
    stack[i].o = argv[i];
    if (!CastObjToItem(args[i], stack+i))
      rb_raise(rb_eRuntimeError, "%s: error in arg %d",
                                    f_commands[index].name, i);
  }
  
  type = f_commands[index].proc(stack);
  if (type == IT_unknown)
    rb_raise(rb_eRuntimeError, "%s: error in result", f_commands[index].name);
  
  result = ItemAsObj(type, stack);
  
  POP_KEEP_REFS
  return result;
}

static VALUE vq_cmdlist (VALUE self) {
  int i, ncmds = sizeof f_commands / sizeof f_commands[0];
  VALUE result = rb_ary_new2(ncmds);
  
  for (i = 0; i < ncmds; ++i)
    rb_ary_store(result, i, rb_str_new2(f_commands[i].name));
    
  return result;
}

static VALUE view_each_keep (VALUE self) {
  int r, rows;
  Item item;
  
  PUSH_KEEP_REFS
  
  item.o = self;
  if (!ObjToItem(IT_view, &item))
    Assert(0);
    
  rows = ViewSize(item.v);
  
  for (r = 0; r < rows; ++r)
    rb_yield(AtRow(self, r));

  POP_KEEP_REFS
  return self;
}

static VALUE row_aref_keep (VALUE self, VALUE index) {
  int col;
  Row_p p;
  View_p v;
  Item item;
  VALUE result;
  
  Data_Get_Struct(self, Row, p);
  Data_Get_Struct(p->view, struct Sequence, v);

  PUSH_KEEP_REFS
  
  if (TYPE(index) == T_FIXNUM)
    col = NUM2INT(index);
  else
    col = StringLookup(STR2CSTR(index), ViewCol(V_Meta(v), MC_name));
  
  Assert(col >= 0);
  item.c = ViewCol(v, col);
  result = ItemAsObj(GetItem(p->row, &item), &item);

  POP_KEEP_REFS
  return result;
}

static VALUE col_aref_keep (VALUE self, VALUE index) {
  Column_p column;
  Item item;
  VALUE result;
  
  Data_Get_Struct(self, Column, column);

  PUSH_KEEP_REFS
  
  item.c = *column;
  result = ItemAsObj(GetItem(NUM2INT(index), &item), &item);

  POP_KEEP_REFS
  return result;
}

static VALUE col_each_keep (VALUE self) {
  Column_p column;
  int r;
  Item item;
  
  Data_Get_Struct(self, Column, column);

  PUSH_KEEP_REFS
  
  for (r = 0; r < column->seq->count; ++r) {
    item.c = *column;
    rb_yield(ItemAsObj(GetItem(r, &item), &item));
  }

  POP_KEEP_REFS
  return self;
}

  static void *WriteDataFun(void *chan, const void *ptr, intptr_t len) {
    Item item;
    item.u.ptr = ptr;
    item.u.len = len;
  	rb_funcall((VALUE) chan, id_lshift, 1, ItemAsObj(IT_bytes, &item));
    return chan;
  }

ItemTypes WriteCmd_VO (Item_p args) {
  intptr_t bytes;
  
  bytes = ViewSave(args[0].v, (void*) args[1].o, NULL, WriteDataFun, 0);
  
  if (bytes < 0)
    return IT_unknown;
    
  args[0].w = bytes;
  return IT_wide;
}

void Init_vlerq (void) {
	VALUE vlerq = rb_define_module("Vlerq");
	rb_define_module_function(vlerq, "cmdlist", vq_cmdlist, 0);
	rb_define_module_function(vlerq, "dispatch", vq_dispatch_keep, -1);

	cRow = rb_define_class_under(vlerq, "Row", rb_cObject);
	rb_define_method(cRow, "_v", row_v, 0);
	rb_define_method(cRow, "_i", row_i, 0);
	rb_define_method(cRow, "[]", row_aref_keep, 1);
	
	cCol = rb_define_class_under(vlerq, "Col", rb_cObject);
  rb_include_module(cCol, rb_mEnumerable);
	rb_define_method(cCol, "[]", col_aref_keep, 1);
	rb_define_method(cCol, "each", col_each_keep, 0);

	cView = rb_define_class("View", rb_cObject);
  rb_define_alloc_func(cView, view_alloc);
  rb_include_module(cView, rb_mEnumerable);
	rb_define_method(cView, "initialize", view_initialize_keep, 1);
	rb_define_method(cView, "each", view_each_keep, 0);

  id_lshift = rb_intern("<<");
}
