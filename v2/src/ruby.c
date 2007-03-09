/* ruby.c -=- Ruby interface to Thrive */

#include <ruby.h>

#define NAME "Ruby"

/* %includeDir<.>% */
/* %includeDir<../noarch>% */
/* %include<thrive.h>% */
/* %include<>% */

static VALUE cThriveRef;

static VALUE
ws_allocate(VALUE klass) {
  return Data_Wrap_Struct(klass, 0, endWork, newWork());
}

static void
ref_free(P p) {
  dropRef(p->p,p->i);
  free(p);
} 

static VALUE
ref_new(P p, I i) {
  P x = ALLOC(B);
  x->p = Work(p);
  x->i = makeRef(p,i);
  return Data_Wrap_Struct(cThriveRef, 0, ref_free, x);
} 

static VALUE
th_get(P p, I i) {
  P w = Work(p);
  if (i < 0) i += Length(p);
  switch (Type(p[i])) {
    case Tn:
      return Qnil;
    case Ti:
      return INT2NUM(p[i].i);
    case Ts:
      return rb_tainted_str_new(strAt(p[i]),lenAt(p[i]));
    case Ta: {
      int j, n = Length(p[i].p);
      VALUE o = rb_ary_new2(n);
      if (n > Limit(p[i].p)) n = Limit(p[i].p);
      for (j = 0; j < n; ++j)
	rb_ary_push(o, th_get(p[i].p,j));
      return o;
    }
  }
  return ref_new(p,i);
}

static void
th_push(P p, VALUE o) {
  switch (TYPE(o)) {
    case T_DATA: {
      P x;
      Data_Get_Struct(o, B, x);
      pushRef(p,x->i);
      break;
    }
    case T_NIL:
      nilPush(p,1);
      break;
    case T_BIGNUM:
    case T_FIXNUM:
      intPush(p,NUM2INT(o));
      break;
    case T_STRING:
      strPush(p,StringValuePtr(o),RSTRING(o)->len);
      break;
    case T_ARRAY: {
      int i, n = RARRAY(o)->len;
      P q = boxPush(p,n);
      for (i = 0; i < n; ++i)
	th_push(q, rb_ary_entry(o, i));
      break;
    }
    default:
      rb_raise (rb_eRuntimeError, "cannot push, unknown type");
  }
}

static VALUE
ws_eval(VALUE self, VALUE v) {
  P w;
  int n;
  Data_Get_Struct(self, B, w);
  v = StringValue(v);
  strPush(w, RSTRING(v)->ptr, RSTRING(v)->len);
  n = findSym(w);
  if (n >= 0) evalSym(w,n);
  return INT2NUM(n);
}

static VALUE
ws_find(VALUE self, VALUE v) {
  P w;
  int n;
  Data_Get_Struct(self, B, w);
  v = StringValue(v);
  strPush(w, RSTRING(v)->ptr, RSTRING(v)->len);
  n = findSym(w);
  /*TODO ugly code, breaks encapsulation of Wv */
  if (n >= Wv.i)
    rb_raise (rb_eRuntimeError, "symbol not found");
  return ref_new(Wv.p,n);
}

static VALUE
ws_load(VALUE self, VALUE v) {
  P w;
  S s = 0;
  M q, r;
  int i, n;
  Data_Get_Struct(self, B, w);
  q = StringValuePtr(v);
  i = RSTRING(v)->len;
  r = q = strcpy(malloc(i+1),q);
  if (*q != '\\' && *q != '#')
    for (;;) {
      s = scanSym(&r);
      if (s == 0) break;
      if (*s != '#') {
	strPush(w,s,~0);
	n = findSym(w);
	if (n >= 0) evalSym(w,n);
	if (runWork(w,0)>>1) {
	  rb_raise (rb_eRuntimeError, "cannot handle callback");
	  break;
	}
      }
    }
  free(q);
  A(s == 0);
  return Qnil;
}

static VALUE
ws_pop(VALUE self) {
  P w;
  Data_Get_Struct(self, B, w);
  return th_get(w,-1);
}

static VALUE
ws_push(int argc, VALUE *argv, VALUE self) {
  P w;
  int i;
  Data_Get_Struct(self, B, w);
  for (i = 0; i < argc; ++i)
    th_push(w,argv[i]);
  return Qnil;
}

static VALUE
ws_run(int argc, VALUE *argv, VALUE self) {
  P w;
  VALUE arg;
  Data_Get_Struct(self, B, w);
  rb_scan_args(argc,argv,"01",&arg);
  if (arg != Qnil) {
    th_push(w,arg);
  }
  return INT2NUM(runWork(w,arg!=Qnil));
}

void Init_thrive() {
  VALUE mThrive = rb_define_module("Thrive");
  VALUE cThriveWs = rb_define_class_under(mThrive, "Workspace", rb_cObject);
  cThriveRef = rb_define_class_under(mThrive, "Reference", rb_cObject);
  rb_define_alloc_func(cThriveWs, ws_allocate);
  rb_define_method(cThriveWs, "eval", ws_eval, 1);
  rb_define_method(cThriveWs, "find", ws_find, 1);
  rb_define_method(cThriveWs, "load", ws_load, 1);
  rb_define_method(cThriveWs, "pop", ws_pop, 0);
  rb_define_method(cThriveWs, "push", ws_push, -1);
  rb_define_method(cThriveWs, "run", ws_run, -1);
}
