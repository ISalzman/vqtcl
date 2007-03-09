/* python.c -=- Python interface to Thrive */

#include <Python.h>

#define NAME "Python"

/* %includeDir<.>% */
/* %includeDir<../noarch>% */
/* %include<thrive.h>% */
/* %include<>% */

typedef struct {
  PyObject_HEAD
  P w;
} WsObject;

typedef struct {
  PyObject_HEAD
  P w;
  I i;
} RefObject;

staticforward PyTypeObject ws_type;
staticforward PyTypeObject ref_type;

#define is_ws(v) ((v)->ob_type == &ws_type)
#define is_ref(v) ((v)->ob_type == &ref_type)
#define wsof(v) (((WsObject*)(v))->w)
#define refof(v) (((RefObject*)(v))->i)

static void ref_dealloc(PyObject *self) {
  dropRef(wsof(self),refof(self));
  PyObject_DEL(self);
} 

statichere PyTypeObject ref_type = {
  PyObject_HEAD_INIT(0)
  0, "thriveref", sizeof (RefObject), 0,
  ref_dealloc,
};

static PyObject* ref_new(P p, I i) {
  RefObject *obj = PyObject_NEW(RefObject,&ref_type);
  if (obj != 0) {
    obj->w = p;
    obj->i = makeRef(p,i);
  }
  return (PyObject*) obj;
} 

static PyObject *thrive_get(P p, I i) {
  P w = Work(p);
  if (i < 0) i += Length(p);
  switch (Type(p[i])) {
    case Tn:
      Py_INCREF(Py_None);
      return Py_None;
    case Ti:
      return PyInt_FromLong((long) p[i].i);
    case Ts:
      return PyString_FromStringAndSize(strAt(p[i]),lenAt(p[i]));
    case Td: {
      int j, n = Length(p[i].p);
      PyObject *o = PyList_New(n);
      if (n > Limit(p[i].p)) n = Limit(p[i].p);
      for (j = 0; j < n; ++j)
	PyList_SetItem(o,j,thrive_get(p[i].p,j));
      return o;
    }
    case Ta: {
      int j, n = Length(p[i].p);
      PyObject *o = PyList_New(n);
      if (n > Limit(p[i].p)) n = Limit(p[i].p);
      for (j = 0; j < n; ++j)
	PyList_SetItem(o,j,thrive_get(p[i].p,j));
      return o;
    }
    case To:
      return ref_new(p,i);
  }
  return 0;
}
static void thrive_push(P p, PyObject *o) {
  if (is_ref(o))
    pushRef(p,refof(o));
  else if (o == Py_None)
    nilPush(p,1);
  else if (PyInt_Check(o))
    intPush(p,PyInt_AS_LONG(o));
  else if (PyString_Check(o))
    strPush(p,PyString_AS_STRING(o),PyString_GET_SIZE(o));
  else if (PySequence_Check(o)) {
    int i, n = PySequence_Size(o);
    P q = boxPush(p,n);
    for (i = 0; i < n; ++i)
      thrive_push(q,PySequence_GetItem(o,i));
  } else
    intPush(p,(I)o); /*XXX unknown type, push the pointer instead */
}
static PyObject *th_find(PyObject *self, PyObject *args) {
  char *s;
  int n;
  P w = wsof(self);
  if (!PyArg_ParseTuple(args,"s:find",&s)) return 0;
  strPush(w,s,~0);
  n = findSym(w);
  /*TODO ugly code, breaks encapsulation of Wv */
  if (n >= Wv.i) {
    PyErr_SetString(PyExc_RuntimeError,"symbol not found");
    return 0;
  }
  return ref_new(Wv.p,n);
}
static PyObject *th_eval(PyObject *self, PyObject *args) {
  char *s;
  int n;
  P w = wsof(self);
  if (!PyArg_ParseTuple(args,"s:eval",&s)) return 0;
  strPush(w,s,~0);
  n = findSym(w);
  if (n >= 0) evalSym(w,n);
  return PyInt_FromLong(n);
}
static PyObject *th_push(PyObject *self, PyObject *args) {
  P w = wsof(self);
  int i, n = PyTuple_GET_SIZE(args);
  for (i = 0; i < n; ++i)
    thrive_push(w,PyTuple_GET_ITEM(args,i));
  Py_INCREF(Py_None);
  return Py_None;
}
static PyObject *th_pop(PyObject *self, PyObject *args) {
  P w = wsof(self);
  PyObject *r = thrive_get(w,-1);
  nilPush(w,-1);
  return r;
}
static PyObject *th_run(PyObject *self, PyObject *args) {
  PyObject *o = 0;
  P w = wsof(self);
  if (!PyArg_ParseTuple(args,"|O!:run",&ref_type,&o)) return 0;
  if (o != 0) thrive_push(w,o);
  return PyInt_FromLong((long) runWork(w,o!=0));
}
static PyObject *th_load(PyObject *self, PyObject *args) {
  S s = 0;
  M q, r;
  int i, n;
  P w = wsof(self);
  if (!PyArg_ParseTuple(args,"s#:load",&q,&i)) return 0;
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
	  PyErr_SetString(PyExc_RuntimeError,"cannot handle callback");
	  break;
	}
      }
    }
  free(q);
  if (s != 0) return 0;
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef
ws_methods[] = {
  { "find",    th_find,    METH_VARARGS, "Lookup a token" },
  { "eval",    th_eval,    METH_VARARGS, "Evaluate a token" },
  { "push",    th_push,    METH_VARARGS, "Push values" },
  { "pop",     th_pop,     METH_VARARGS, "Pop top of stack" },
  { "run",     th_run,     METH_VARARGS, "Run loop to execute code" },
  { "load",    th_load,    METH_VARARGS, "Load words into workspace" },
  { 0 }
};

static void ws_dealloc(PyObject *self) {
  endWork(wsof(self));
  PyObject_DEL(self);
} 
static PyObject* ws_getattr(PyObject *self, char *name) {
  return Py_FindMethod(ws_methods,self,name);
}

statichere PyTypeObject ws_type = {
  PyObject_HEAD_INIT(0)
  0, "thrive", sizeof (WsObject), 0,
  ws_dealloc,
  0,
  ws_getattr,
};

static PyObject* thrive_new(PyObject *self, PyObject *args) {
  WsObject *obj = PyObject_NEW(WsObject,&ws_type);
  if (obj != 0) wsof(obj) = newWork();
  return (PyObject*) obj;
} 

static PyMethodDef thriveMethods[] = {
  { "workspace",  thrive_new,  METH_VARARGS, "Create a new workspace" },
  { 0 }
};

void initthrive(void) {
  PyObject *m = Py_InitModule("thrive",thriveMethods);
  PyModule_AddIntConstant(m,"VERSION",VERSION);
  ws_type.ob_type = &PyType_Type;
  ref_type.ob_type = &PyType_Type;
}
