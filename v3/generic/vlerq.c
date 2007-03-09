/* vlerq.c

  TODO
    - solidify so no crashes are possible from Tcl
    - implement a new commit system, using differential commit/extend

  MAYBE
    - simplify from Tcl_GetIndexFromObj for col names to <col,#> Tcl_Obj's?
*/

#include <tcl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef unsigned char u_char;
typedef __int64 int64_t;
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#if __ARMEB__
#define _BIG_ENDIAN 1
#endif
#ifdef __sparc__
#define VALUES_MUST_BE_ALIGNED 1
#endif

/* stub interface code, removes the need to link with libtclstub*.a */
#ifdef USE_TCL_STUBS
#include "stubtcl.h"
#else
#define MyInitStubs(x) 1
#endif

#ifdef NDEBUG
#define A(x)
#define D Z
#else
#include <assert.h>
#define A(x) assert(x)
#define D
#endif
#define Z if (1) ; else

  /* forward */
static Tcl_ObjType view_type;

typedef struct Shared {
  Tcl_Obj* stash;
  Tcl_Obj* none;
  Tcl_Obj* hash;
  Tcl_Obj* stype;
  Tcl_Obj* vtype;
  Tcl_Obj* view;
  Tcl_Obj* vops;
  Tcl_Obj* mdef;
  Tcl_Obj* vdata;
  Tcl_Obj* remap;
  Tcl_Obj* at;
  Tcl_Obj* meta;
  Tcl_Obj* mapf;
  Tcl_Obj** tinies;
  struct View* mmeta;
  struct View* zmeta;
  Tcl_Interp* cip;
} Shared;

#if NO_THREAD_CALLS
static Shared* get_shared() {
  static Shared data;
  return &data;
}
#define unget_shared Tcl_CreateExitHandler
#else
static Tcl_ThreadDataKey tls_data;
#define get_shared() ((Shared*) Tcl_GetThreadData(&tls_data, sizeof(Shared)))
#define unget_shared Tcl_CreateThreadExitHandler
#endif

static void dupFakeIntRep(Tcl_Obj *o, Tcl_Obj *p) {
  D if (o->typePtr != 0) printf("dfir: %s\n", o->typePtr->name);
  A(0);
  Tcl_Panic("dupFakeIntRep");
}
static int setFakeFromAnyRep(Tcl_Interp *ip, Tcl_Obj *o) {
  A(0);
  Tcl_Panic("setFakeFromAnyRep");
  return TCL_ERROR;
}

static Tcl_Obj* incr_ref(Tcl_Obj* obj) {
  A(obj != 0);
  Tcl_IncrRefCount(obj);
  return obj;
}
static void drop_ref(Tcl_Obj* obj) {
  A(obj != 0);
  if (obj->refCount <= 0)
    Tcl_DecrRefCount(obj);
}

static int l_length(Tcl_Obj* obj) {
  int n, e;
  A(obj->typePtr != &view_type);
  e = Tcl_ListObjLength(0, obj, &n);
  A(e == TCL_OK);
  return n;
}
static Tcl_Obj* l_index(Tcl_Obj* obj, int index) {
  Tcl_Obj* p;
  int e;
  A(obj->typePtr != &view_type);
  e = Tcl_ListObjIndex(0, obj, index, &p);
  A(e == TCL_OK);
  return p;
}
static Tcl_Obj* l_append(Tcl_Obj* obj, Tcl_Obj* elt) {
  int e;
  A(obj->typePtr != &view_type);
  e = Tcl_ListObjAppendElement(0, obj, elt); A(e == TCL_OK);
  return elt;
}
static Tcl_Obj* new_list4(Tcl_Obj* a, Tcl_Obj* b, Tcl_Obj* c, Tcl_Obj* d) {
  Tcl_Obj* vec[4];
  A(a != 0 && b != 0);
  vec[0] = a; vec[1] = b; vec[2] = c; vec[3] = d;
  return Tcl_NewListObj(c == 0 ? 2 : d == 0 ? 3 : 4, vec);
}
static Tcl_Obj* new_int(int i) {
  return -128 <= i && i <= 127 ? get_shared()->tinies[i] : Tcl_NewIntObj(i);
}
static Tcl_Obj* make_atList(Tcl_Obj* parent, int row, int col) {
  return new_list4(get_shared()->at, parent, new_int(row), new_int(col));
}
static int int_32be(const u_char* p) {
  return (((char) p[0]) << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static int top_bit(unsigned v) {
#define Vn(x) (v < (1<<x))
  return Vn(16) ? Vn( 8) ? Vn( 4) ? Vn( 2) ? Vn( 1) ? (int)v-1 : 1
                                           : Vn( 3) ?  2 :  3
                                  : Vn( 6) ? Vn( 5) ?  4 :  5
                                           : Vn( 7) ?  6 :  7
                         : Vn(12) ? Vn(10) ? Vn( 9) ?  8 :  9
                                           : Vn(11) ? 10 : 11
                                  : Vn(14) ? Vn(13) ? 12 : 13
                                           : Vn(15) ? 14 : 15
                : Vn(24) ? Vn(20) ? Vn(18) ? Vn(17) ? 16 : 17
                                           : Vn(19) ? 18 : 19
                                  : Vn(22) ? Vn(21) ? 20 : 21
                                           : Vn(23) ? 22 : 23
                         : Vn(28) ? Vn(26) ? Vn(25) ? 24 : 25
                                           : Vn(27) ? 26 : 27
                                  : Vn(30) ? Vn(29) ? 28 : 29
                                           : Vn(31) ? 30 : 31;
#undef Vn
}
static int min_width(int lo, int hi) {
  if (lo > 0) lo = 0;
  if (hi < 0) hi = 0;
  lo = "04444444455555555666666666666666"[top_bit(~lo)+2] & 7;
  hi = "01233444555555556666666666666666"[top_bit(hi)+1] & 7;
  return lo > hi ? lo : hi;
}

/* - - -  LAZY  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define LAZY_OBJ(obj) (*(Tcl_Obj**) &(obj)->internalRep.twoPtrValue.ptr1)
#define LAZY_PTR(obj) (*(const char**) &(obj)->internalRep.twoPtrValue.ptr2)

static void freeLazyIntRep(Tcl_Obj *o) {
  Tcl_DecrRefCount(LAZY_OBJ(o));
}
static void updateLazyStrRep(Tcl_Obj *o) {
  const char* p = LAZY_PTR(o);
  A(o->length == strlen(p));
  o->bytes = strcpy(ckalloc(o->length + 1), p);
}

static Tcl_ObjType lazy_type = {
  "lazy", freeLazyIntRep, dupFakeIntRep, updateLazyStrRep, setFakeFromAnyRep
};

static Tcl_Obj* make_lazy(Tcl_Obj* owner, const char* p, int n) {
  Tcl_Obj* o = Tcl_NewObj();
  LAZY_OBJ(o) = incr_ref(owner);
  LAZY_PTR(o) = p;
  o->typePtr = &lazy_type;
  Tcl_InvalidateStringRep(o);
  o->length = n; /* TODO: is this value safe while there is no string rep? */
  return o;
}

/* - - -  VALUES  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum Vtype {
  Vt_nil, Vt_int, Vt_wid, Vt_dbl, Vt_lzy, Vt_str, Vt_bin, Vt_obj
} Vtype;

typedef struct Value {
  union {
    int i; float f; int64_t q; double d; const char* p; Tcl_Obj* o; char b[8];
  } u;
  int n, c;
  Tcl_Obj *o, *p;
  Vtype t;
} Value;

static Tcl_Obj* val_toObj(Vtype type, Value* vp) {
  switch (type) {
    case Vt_nil:  vp->u.o = Tcl_NewObj(); break;
    case Vt_int:  vp->u.o = new_int(vp->u.i); break;
    case Vt_wid:  vp->u.o = Tcl_NewWideIntObj(vp->u.q); break;
    case Vt_dbl:  vp->u.o = Tcl_NewDoubleObj(vp->u.d); break;
    case Vt_lzy:  vp->u.o = make_lazy(vp->o, vp->u.p, vp->n); break;
    case Vt_str:  vp->u.o = Tcl_NewStringObj(vp->u.p, vp->n); break;
    case Vt_bin:  vp->u.o = Tcl_NewByteArrayObj((const u_char*) vp->u.p, vp->n);
    case Vt_obj:  break;
    default:      A(0);
  }
  vp->t = Vt_obj;
  return vp->u.o;
}
static int val_toInt(Vtype type, Value* vp) {
  int e;
  Tcl_Obj* o;
  switch (type) {
    default:      val_toObj(type, vp); /* fall through */
    case Vt_obj:  o = vp->u.o;
                  e = Tcl_GetIntFromObj(0, o, &vp->u.i); A(e == TCL_OK);
                  drop_ref(o);
    case Vt_int:  break;
  }
  vp->t = Vt_int;
  return vp->u.i;
}
static int64_t val_toQuad(Vtype type, Value* vp) {
  int e;
  Tcl_Obj* o;
  switch (type) {
    default:      val_toObj(type, vp); /* fall through */
    case Vt_obj:  o = vp->u.o;
                  e = Tcl_GetWideIntFromObj(0, o, &vp->u.q); A(e == TCL_OK);
                  drop_ref(o);
    case Vt_wid:  break;
  }
  vp->t = Vt_wid;
  return vp->u.q;
}
static double val_toReal(Vtype type, Value* vp) {
  int e;
  Tcl_Obj* o;
  switch (type) {
    default:      val_toObj(type, vp); /* fall through */
    case Vt_obj:  o = vp->u.o;
                  e = Tcl_GetDoubleFromObj(0, o, &vp->u.d); A(e == TCL_OK);
                  drop_ref(o);
    case Vt_dbl:  break;
  }
  vp->t = Vt_dbl;
  return vp->u.d;
}

/* - - -  BUFFERS  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct Overflow {
  struct Overflow* p;
  char b[4096];
} Overflow;

typedef struct Buffer {
  union { char* c; int* i; const void** p; } fill;
  char* limit;
  Overflow* head;
  char* ofill;
  int ocount;
  char* result;
  char buf[256];
  char slack[8];
} Buffer;

#define BUF_BYTES(b) ((b).ocount + ((b).fill.c - (b).buf))
#define BUF_ADD_CHAR(b,x) \
          { char _c = x; \
            if ((b).fill.c < (b).limit) *(b).fill.c++ = _c; \
              else buf_add(&(b), &_c, sizeof _c); }
#define BUF_ADD_INT(b,x) \
          { int _i = x; \
            if ((b).fill.c < (b).limit) *(b).fill.i++ = _i; \
              else buf_add(&(b), &_i, sizeof _i); }
#define BUF_ADD_PTR(b,x) \
          { const void* _p = x; \
            if ((b).fill.c < (b).limit) *(b).fill.p++ = _p; \
              else buf_add(&(b), &_p, sizeof _p); }

static void buf_init(Buffer* bp) {
  bp->fill.c = bp->buf;
  bp->limit = bp->buf + sizeof bp->buf;
  bp->head = 0;
  bp->ofill = 0;
  bp->ocount = 0;
  bp->result = 0;
};
static void buf_free(Buffer* bp) {
  while (bp->head != 0) {
    Overflow* op = bp->head;
    bp->head = op->p;
    ckfree((char*) op);
  }
  if (bp->result != 0)
    ckfree(bp->result);
}
static void buf_add(Buffer* bp, const void* data, int len) {
  int n;
  while (len > 0) {
    if (bp->fill.c >= bp->limit) {
      if (bp->head == 0 || bp->ofill >= bp->head->b + sizeof bp->head->b) {
        Overflow* op = (Overflow*) ckalloc(sizeof(Overflow));
        op->p = bp->head;
        bp->head = op;
        bp->ofill = op->b;
      }
      memcpy(bp->ofill, bp->buf, sizeof bp->buf);
      bp->ofill += sizeof bp->buf;
      bp->ocount += sizeof bp->buf;
      n = bp->fill.c - bp->slack;
      memcpy(bp->buf, bp->slack, n);
      bp->fill.c = bp->buf + n;
    }
    n = len;
    if (n > bp->limit - bp->fill.c)
      n = bp->limit - bp->fill.c; /* TODO: copy big chunks to overflow */
    A(n > 0);
    memcpy(bp->fill.c, data, n);
    bp->fill.c += n;
    data += n;
    len -= n;
  }
}
static char* buf_asPtr(Buffer* bp, int fast) {
  int n = bp->fill.c - bp->buf, len = bp->ocount + n;
  if (fast && n == len)
    return bp->buf;
  if (bp->result == 0)
    bp->result = ckalloc(len);
  len -= n;
  memcpy(bp->result + len, bp->buf, n);
  if (bp->head != 0) {
    Overflow* curr = bp->head;
    n = bp->ofill - curr->b;
    do {
      len -= n;
      memcpy(bp->result + len, curr->b, n);
      curr = curr->p;
      n = sizeof bp->head->b;
    } while (curr != 0);
  }
  A(len == 0);
  return bp->result;
}
static Tcl_Obj* buf_asList(Buffer* bp) {
  int ac = BUF_BYTES(*bp) / sizeof(Tcl_Obj*);
  Tcl_Obj* o = Tcl_NewListObj(ac, (Tcl_Obj**) buf_asPtr(bp, 1));
  buf_free(bp);
  return o;
}


/* - - -  MAPS  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct Map {
  char* ptr;
  int off; /* TODO: drop, use a shared segment of a second map instead */
  int len;
  Tcl_Obj* orig;
} Map;

  /* forward */
static Tcl_ObjType map_type;

#define MAP_OBJ(obj) (*(Map**) &(obj)->internalRep.otherValuePtr)

static Map* as_map(Tcl_Obj* obj) {
  if (obj->typePtr != &map_type) {
    int e = Tcl_ConvertToType(0, obj, &map_type);
    D if (e != TCL_OK) {
      Tcl_Interp* cip = get_shared()->cip;
      e = Tcl_ConvertToType(cip, obj, &map_type);
      A(e != TCL_OK);
      printf("as_map? %s\n", Tcl_GetStringResult(cip));
    }
    A(e == TCL_OK);
  }
  return MAP_OBJ(obj);
}
static void freeMapIntRep(Tcl_Obj *o) {
  Map* m = MAP_OBJ(o);
  if (m->off >= 0) {
#if WIN32
    UnmapViewOfFile(m->ptr - m->off);
#else
#ifndef NDEBUG
    int e = 
#endif
      munmap(m->ptr - m->off, m->len + m->off);
      A(e == 0);
#endif
  }
  Tcl_DecrRefCount(m->orig);
  ckfree((char*) m);
}
static int setMapFromAnyRep(Tcl_Interp *ip, Tcl_Obj *o) {
  Map* m;
  int n;
  A(o->typePtr != &view_type);
  if (Tcl_ListObjLength(ip, o, &n) != TCL_OK)
    return TCL_ERROR;
  A(n == 2);
  m = (Map*) ckalloc(sizeof(Map));
  m->orig = incr_ref(Tcl_DuplicateObj(o));
  if (strcmp(Tcl_GetString(l_index(m->orig, 0)), "::vlerq::mapf") == 0) {
    const char* fn = Tcl_GetString(l_index(m->orig, 1));
#if WIN32
    DWORD n;
    HANDLE h, f = CreateFile(fn,GENERIC_READ,FILE_SHARE_READ,0,
    	  		                 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    A(f != INVALID_HANDLE_VALUE);
	  h = CreateFileMapping(f,0,PAGE_READONLY,0,0,0);
	  A(h != INVALID_HANDLE_VALUE);
    n = GetFileSize(f,0);
    m->ptr = MapViewOfFile(h,FILE_MAP_READ,0,0,n);
    A(m->ptr != 0);
    m->len = n;
    CloseHandle(h);
    CloseHandle(f);
#else
    struct stat sb;
    int e, fd = open(fn, O_RDONLY); A(fd != -1);
    e = fstat(fd, &sb); A(e == 0);
    m->ptr = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    A(m->ptr != MAP_FAILED);
    m->len = sb.st_size;
    e = close(fd); A(e == 0);
#endif
    A(m->len > 24);
    A(int_32be((const u_char*) m->ptr + m->len - 16) == 0x80000000);
    m->off = m->len - int_32be((const u_char*) m->ptr + m->len - 12) - 16;
    m->ptr += m->off;
    m->len -= m->off;
  } else { /* data bytes */
    m->ptr = (char*) Tcl_GetByteArrayFromObj(l_index(m->orig, 1), &m->len);
    m->off = -1;
  }
  if (o->typePtr != 0 && o->typePtr->freeIntRepProc != 0)
    o->typePtr->freeIntRepProc(o);
  MAP_OBJ(o) = m;
  o->typePtr = &map_type;
  return TCL_OK;
}

static Tcl_ObjType map_type = {
  "map", freeMapIntRep, dupFakeIntRep, 0, setMapFromAnyRep
};

/* - - -  COLUMNS  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct Column Column;
typedef Vtype (*Getter)(Column*,int,Value*);
typedef void (*Cleaner)(Column*);

struct Column {
  char* ptr;
  Tcl_Obj* aux;
  Getter getter;
  Cleaner cleaner;
  int refs, count;
};

typedef struct Colref {
  Column* c;
  int i;
} Colref;

#define MAKE_COL(t,a,p,c,f) ((t*) make_col(sizeof(t),a,p,c,(Getter)f))

  /* forward */
static Tcl_ObjType col_type;
static void col_convert(Colref*,Getter,int,int);
static Vtype g_obj(Column*,int,Value*);

static Column* col_incRef(Column* c) {
  A(c != 0);
  ++c->refs;
  return c;
}
static void col_decRef(Column* c) {
  if (c != 0 && --c->refs <= 0) {
    c->cleaner(c);
    ckfree((char*) c);
  }
}

static void std_cleaner(Column* c) {
  Tcl_DecrRefCount(c->aux);
}

static Column* make_col(int w, Tcl_Obj* aux, char* ptr, int count, Getter cf) {
  Column* c = (Column*) ckalloc(w);
  A(w >= sizeof(Column));
  c->aux = incr_ref(aux);
  c->ptr = ptr;
  c->count = count;
  c->getter = cf;
  c->cleaner = std_cleaner;
  c->refs = 0;
  return c;
}

#define COL_REF(obj) (*(Colref*) &(obj)->internalRep.twoPtrValue.ptr1)

static Colref as_col(Tcl_Obj* obj) {
  if (obj->typePtr != &col_type) {
    int e = Tcl_ConvertToType(0, obj, &col_type);
    D if (e != TCL_OK) {
      Tcl_Interp* cip = get_shared()->cip;
      e = Tcl_ConvertToType(cip, obj, &col_type);
      A(e != TCL_OK);
      printf("as_col? %s\n", Tcl_GetStringResult(cip));
    }
    A(e == TCL_OK);
  }
  return COL_REF(obj);
}

static void freeColIntRep(Tcl_Obj *o) {
  col_decRef(COL_REF(o).c);
}
static void updateColStrRep(Tcl_Obj *o) {
  char* s;
  Colref cr = as_col(o);
  col_convert(&cr, g_obj, 0, 1);
  s = Tcl_GetStringFromObj(cr.c->aux, &o->length);
  o->bytes = strcpy(ckalloc(o->length + 1), s);
  col_decRef(cr.c);
}
static int setColFromAnyRep(Tcl_Interp *ip, Tcl_Obj *o) {
  int ac;
  Tcl_Obj *q, **av;
  Column* c;
  A(o->typePtr != &view_type);
  q = Tcl_DuplicateObj(o);
  if (Tcl_ListObjGetElements(ip, q, &ac, &av) != TCL_OK)
    return TCL_ERROR;
  c = MAKE_COL(Column, q, (char*) av, ac, g_obj);
  if (o->typePtr != 0 && o->typePtr->freeIntRepProc != 0)
    o->typePtr->freeIntRepProc(o);
  COL_REF(o).c = col_incRef(c);
  COL_REF(o).i = -1;
  o->typePtr = &col_type;
  return TCL_OK;
}

static Tcl_ObjType col_type = {
  "col", freeColIntRep, dupFakeIntRep, updateColStrRep, setColFromAnyRep
};

static Vtype g_i0(Column* col, int row, Value* out) {
  out->u.i = 0;
  return Vt_int;
}
static Vtype g_i1(Column* col, int row, Value* out) {
  out->u.i = (col->ptr[row>>3] >> (row&7)) & 1;
  return Vt_int;
}
static Vtype g_i2(Column* col, int row, Value* out) {
  out->u.i = (col->ptr[row>>2] >> 2*(row&3)) & 3;
  return Vt_int;
}
static Vtype g_i4(Column* col, int row, Value* out) {
  out->u.i = (col->ptr[row>>1] >> 4*(row&1)) & 15;
  return Vt_int;
}
static Vtype g_i8(Column* col, int row, Value* out) {
  out->u.i = (signed char) col->ptr[row];
  return Vt_int;
}

#if VALUES_MUST_BE_ALIGNED
static Vtype g_i16(Column* col, int row, Value* out) {
  const u_char* p = (const u_char*) ((const short*) col->ptr + row);
#if _BIG_ENDIAN
  out->u.i = (((signed char) p[0]) << 8) | p[1];
#else
  out->u.i = (((signed char) p[1]) << 8) | p[0];
#endif
  return Vt_int;
}
static Vtype g_i32(Column* col, int row, Value* out) {
  const char* p = col->ptr + row * 4;
  int i;
  for (i = 0; i < 4; ++i)
    out->u.c[i] = p[i];
  return Vt_int;
}
static Vtype g_i64(Column* col, int row, Value* out) {
  const char* p = col->ptr + row * 8;
  int i;
  for (i = 0; i < 8; ++i)
    out->u.c[i] = p[i];
  return Vt_wid;
}
static Vtype g_f32(Column* col, int row, Value* out) {
  g_i32(col, row, out);
  out->u.d = out->u.f;
  return Vt_dbl;
}
static Vtype g_f64(Column* col, int row, Value* out) {
  g_i64(col, row, out);
  return Vt_dbl;
}
#else
static Vtype g_i16(Column* col, int row, Value* out) {
  out->u.i = ((short*) col->ptr)[row];
  return Vt_int;
}
static Vtype g_i32(Column* col, int row, Value* out) {
  out->u.i = ((const int*) col->ptr)[row];
  return Vt_int;
}
static Vtype g_i64(Column* col, int row, Value* out) {
  out->u.q = ((const int64_t*) col->ptr)[row];
  return Vt_wid;
}
static Vtype g_f32(Column* col, int row, Value* out) {
  out->u.i = ((const int*) col->ptr)[row];
  out->u.d = out->u.f;
  return Vt_dbl;
}
static Vtype g_f64(Column* col, int row, Value* out) {
  out->u.q = ((const int64_t*) col->ptr)[row];
  return Vt_dbl;
}
#endif

static Vtype g_i16r(Column* col, int row, Value* out) {
  const u_char* p = (const u_char*) ((const short*) col->ptr + row);
#if _BIG_ENDIAN
  out->u.i = (((signed char) p[1]) << 8) | p[0];
#else
  out->u.i = (((signed char) p[0]) << 8) | p[1];
#endif
  return Vt_int;
}
static Vtype g_i32r(Column* col, int row, Value* out) {
  const char* p = col->ptr + row * 4;
  int i;
  for (i = 0; i < 4; ++i)
    out->u.b[i] = p[3-i];
  return Vt_int;
}
static Vtype g_i64r(Column* col, int row, Value* out) {
  const char* p = col->ptr + row * 8;
  int i;
  for (i = 0; i < 8; ++i)
    out->u.b[i] = p[7-i];
  return Vt_wid;
}
static Vtype g_f32r(Column* col, int row, Value* out) {
  g_i32r(col, row, out);
  out->u.d = out->u.f;
  return Vt_dbl;
}
static Vtype g_f64r(Column* col, int row, Value* out) {
  g_i64r(col, row, out);
  return Vt_dbl;
}

static Vtype g_obj(Column* col, int row, Value* out) {
  A(0 <= row && row < col->count);
  A(col->ptr != 0);
  out->u.o = ((Tcl_Obj**) col->ptr)[row];
  return Vt_obj;
}
static Vtype g_svec(Column* col, int row, Value* out) {
  out->u.p = ((const char**) col->ptr)[row];
  out->n = strlen(out->u.p);
  if (*out->u.p) {
    out->o = col->aux;
    return Vt_lzy;
  }
  return Vt_nil;
}
static Vtype g_iota(Column* col, int row, Value* out) {
  out->u.i = row;
  return Vt_int;
}

static Tcl_Obj* new_col(Column* c, int i) {
  Tcl_Obj* o = Tcl_NewObj();
  COL_REF(o).c = col_incRef(c);
  COL_REF(o).i = i;
  o->typePtr = &col_type;
  Tcl_InvalidateStringRep(o);
  return o;
}

static int col_str(Column* c, int row, Value* vp) {
  Tcl_Obj* o;
  Vtype t = c->getter(c, row, vp); 
  switch (t) {
    case Vt_nil: case Vt_lzy: case Vt_str: break;
    default:      val_toObj(t, vp); /* fall through */
    case Vt_obj:  o = vp->u.o;
                  if (o->typePtr == &lazy_type) {
                    vp->u.p = LAZY_PTR(o);
                    vp->o = LAZY_OBJ(o);
                    vp->n = o->length;
Z if (vp->n != strlen(vp->u.p))
    printf("lazy? strlen was %d, expected %d\n", vp->n, (int) strlen(vp->u.p));
vp->n = strlen(vp->u.p); /* FIXME: length != strlen! */
                    A(vp->n == strlen(vp->u.p));
                    t = Vt_lzy;
                    break;
                  } /* fall through */
                  vp->u.p = Tcl_GetStringFromObj(o, &vp->n);
                  drop_ref(o);
                  t = Vt_str;
  }
  vp->t = t;
  return vp->n;
}
static void col_convert(Colref* crp, Getter getter, int start, int step) {
  int i;
  Value vb;
  Column* c = crp->c;
  char* ptr = c->ptr;
  int count = c->count / step;
  Tcl_Obj* aux = 0;
  vb.c = crp->i;
  if (getter == g_i32) {
    aux = Tcl_NewObj();
    ptr = (char*) Tcl_SetByteArrayLength(aux, count * sizeof(int));
    for (i = 0; i < count; ++i) 
      ((int*) ptr)[i] = val_toInt(c->getter(c, start + step * i, &vb), &vb); 
  } else if (getter == g_i64) {
    aux = Tcl_NewObj();
    ptr = (char*) Tcl_SetByteArrayLength(aux, count * sizeof(int64_t));
    for (i = 0; i < count; ++i) 
      ((int64_t*) ptr)[i] = val_toQuad(c->getter(c, start + step*i, &vb), &vb); 
  } else if (getter == g_f32) {
    aux = Tcl_NewObj();
    ptr = (char*) Tcl_SetByteArrayLength(aux, count * sizeof(float));
    for (i = 0; i < count; ++i) 
      ((float*) ptr)[i] = val_toReal(c->getter(c, start + step * i, &vb), &vb); 
  } else if (getter == g_f64) {
    aux = Tcl_NewObj();
    ptr = (char*) Tcl_SetByteArrayLength(aux, count * sizeof(double));
    for (i = 0; i < count; ++i) 
      ((double*) ptr)[i] = val_toReal(c->getter(c, start + step * i, &vb), &vb); 
  } else if (getter == g_svec) {
    int bytes;
    char** names;
    aux = Tcl_NewObj();
    /* pre-flight to determine total buffer size */
    bytes = (count+1) * sizeof(char*);
    for (i = 0; i < count; ++i)
      bytes += col_str(c, start + step * i, &vb) + 1;
    ptr = (char*) Tcl_SetByteArrayLength(aux, bytes);
    names = (char**) ptr;
    /* copy strings and set up the index vector */
    bytes = (count+1) * sizeof(char*);
    for (i = 0; i < count; ++i) {
      vb.n = col_str(c, start + step * i, &vb) + 1; 
      A(vb.u.p[vb.n-1] == 0);
      names[i] = memcpy(ptr + bytes, vb.u.p, vb.n); 
      bytes += vb.n; 
    }
    names[i] = 0;
  } else if (getter == g_obj) {
    int e;
    Buffer buf;
    Tcl_Obj** av;
    buf_init(&buf);
    for (i = 0; i < count; ++i) {
      Vtype t = c->getter(c, start + step * i, &vb);
      BUF_ADD_PTR(buf, t == Vt_obj ? vb.u.o : val_toObj(t, &vb));
    }
    aux = buf_asList(&buf);
    e = Tcl_ListObjGetElements(0, aux, &count, &av);
    A(e == TCL_OK);
    ptr = (char*) av;
  } else
    A(0);
  crp->c = MAKE_COL(Column, aux, ptr, count, getter);
}

static int get_v(char** cp) {
  signed char b;
  int v = 0;
  do {
    b = *(*cp)++;
    v = (v << 7) + b;
  } while (b >= 0);
  return v + 128;
}
static int get_p(char** cp) {
  int n = get_v(cp);
  if (n > 0 && get_v(cp) == 0)
    *cp += n;
  return n;
}

static int* make_intCol(int n, Column** cp) {
  Tcl_Obj* o = Tcl_NewObj();
  int* p = (int*) Tcl_SetByteArrayLength(o, n * sizeof(int));
  *cp = MAKE_COL(Column, o, (char*) p, n, g_i32);
  return p;
}
static void resize_intCol(Column* c) {
  c->ptr = (char*) Tcl_SetByteArrayLength(c->aux, c->count * sizeof(int));
}
static Tcl_Obj* buf_asIntCol(Buffer* bp, int max) {
  Column* c;
  int n = BUF_BYTES(*bp) / sizeof(int);
  if (n != max) {
    bp->result = (char*) make_intCol(n, &c);
    buf_asPtr(bp, 0);
    bp->result = 0;
  } else
    c = MAKE_COL(Column, Tcl_NewObj(), 0, n, g_iota);
  buf_free(bp);
  return new_col(c, -1);
}

/* - - -  VIEWS  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct View View;
typedef int (*Sizer)(View*);

struct View {
  View* meta;
  Sizer sizer;
  int refs, size;
  Tcl_Interp* ip;
  Colref cols[1];    /* must be last slot */
};

typedef struct Origin {
  Tcl_Obj* def;
  Tcl_Obj* deps;
} Origin;

  /* forward */
static Tcl_ObjType view_type;
static View* as_view(Tcl_Obj*);
static Tcl_Obj* view_col(Tcl_Obj*,View*,int);
static View* sub_view(Tcl_Obj*,int,View*);
static void view_decRef(View*);

static int view_size(View* v) {
  if (v->size < 0)
    v->size = v->sizer(v);
  return v->size;
}
static int view_width(View* v) {
  return view_size(v->meta);
}
static Vtype view_get(View* v, int row, Value* vp) {
  Vtype t;
  Column* c;
  int col = vp->c;
  A(0 <= col && col < view_width(v));
  c = v->cols[col].c;
  vp->c = v->cols[col].i;
  t = c->getter(c, row, vp);
  vp->c = col;
  return t;
}
static Tcl_Obj* view_at(Tcl_Obj* view, View* v, int row, int col) {
  Value vb;
  vb.c = col;
  vb.o = view;
  Vtype t = view_get(v, row, &vb);
///printf(" va %p %p %d %d -> t %d\n", view, v, row, col, t); fflush(stdout);
  return t == Vt_obj ? vb.u.o : val_toObj(t, &vb);
}
static void drop_deps(Tcl_Obj*);

static int std_sizer(View* v) {
  puts("std_sizer?");
  A(0);
  return -1;
}

static View* view_incRef(View* v) {
  A(v != 0);
  ++v->refs;
  return v;
}
static void view_decRef(View* v) {
  A(v != 0);
  if (--v->refs <= 0) {
    int i = 0, nc = view_width(v);
    for (i = 0; i <= nc; ++i)
      col_decRef(v->cols[i].c);
    view_decRef(v->meta);
    ckfree((char*) v);
  }
}

#define VIEW_REF(obj) (*(View**) &(obj)->internalRep.twoPtrValue.ptr1)
#define VIEW_ORG(obj) (*(Origin**) &(obj)->internalRep.twoPtrValue.ptr2)

static Tcl_Obj* new_view(View* v, Tcl_Obj* g) {
  Tcl_Obj* o = Tcl_NewObj();
  Origin* d = (Origin*) ckalloc(sizeof(Origin));
  d->def = incr_ref(g != 0 ? g : get_shared()->none);
  d->deps = 0;
  VIEW_REF(o) = view_incRef(v);
  VIEW_ORG(o) = d;
  o->typePtr = &view_type;
  Tcl_InvalidateStringRep(o);
  return o;
}
static char* dep_tracer(ClientData cd, Tcl_Interp* ip, const char* name1, const char* name2, int flags) {
  Tcl_Obj *d, *o = (Tcl_Obj*) cd;
  if (o->typePtr == &view_type) {
    drop_deps(o);
    /* TODO: clean up this horrible hack to get rid of the view internal rep */
    d = incr_ref(VIEW_ORG(o)->def);
    A(o->typePtr->freeIntRepProc != 0);
    o->typePtr->freeIntRepProc(o);
    Tcl_InvalidateStringRep(o);
    o->typePtr = d->typePtr; d->typePtr = 0;
    o->internalRep = d->internalRep;
    A(d->refCount == 1);
    Tcl_DecrRefCount(d);
  }
  return 0;
}
static void drop_deps(Tcl_Obj* obj) {
  if (obj->typePtr == &view_type) {
    Tcl_Obj* d = VIEW_ORG(obj)->deps;
    if (d != 0) {
      int e, i, oc;
      Tcl_Obj **ov;
      Tcl_Interp* ip = VIEW_REF(obj)->ip;
      e = Tcl_ListObjGetElements(0, d, &oc, &ov); A(e == TCL_OK);
      for (i = 0; i < oc; ++i) {
        Tcl_UntraceVar2(ip, Tcl_GetString(ov[i]), 0,
                        TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                        dep_tracer, (ClientData) obj);
      }
      Tcl_DecrRefCount(d);
//    VIEW_ORG(obj)->deps = 0;
      VIEW_ORG(obj)->deps = incr_ref(Tcl_NewObj());
    }
  }
}
static void add_deps(Tcl_Obj* src, Tcl_Obj* dest) {
  Tcl_Obj* s;
  A(src->typePtr == &view_type);
  s = VIEW_ORG(src)->deps;
  if (s != 0) {
    int e, i, sc;
    Tcl_Obj **sv, *d = VIEW_ORG(dest)->deps;
    Tcl_Interp* ip = VIEW_REF(dest)->ip;
    if (d == 0)
      d = VIEW_ORG(dest)->deps = incr_ref(Tcl_NewListObj(0, 0));
    e = Tcl_ListObjGetElements(0, s, &sc, &sv); A(e == TCL_OK);
    for (i = 0; i < sc; ++i) {
      e = Tcl_ListObjAppendElement(0, d, sv[i]); A(e == TCL_OK);
      e = Tcl_TraceVar2(ip, Tcl_GetString(sv[i]), 0,
                        TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                        dep_tracer, (ClientData) dest); A(e == TCL_OK);
    }
  }
}
static Tcl_Obj* new_viewDep(View* v, Tcl_Obj* g, int mask) {
  int i;
  Tcl_Obj *o;
  A(g != 0);
  o = new_view(v, g);
  for (i = 0; mask; ++i) {
    if (mask & 1) {
      Tcl_Obj* parent = l_index(g, i);
      A(parent != 0);
      add_deps(parent, o);
    }
    mask >>= 1;
  }
  return o;
}
static Tcl_Obj* new_viewAt(View* v, Tcl_Obj* o, int rn, int cn) {
///printf(" nva %p %p %d %d\n", v, o, rn, cn); fflush(stdout);
///A(cn != 2);
  return new_viewDep(v, make_atList(o, rn, cn), 0x02);
}

static Vtype g_view(Column* col, int row, Value* out) {
  View* v;
  A(0 <= row && row < col->count);
  A(col->ptr != 0);
  v = ((View**) col->ptr)[row]; A(v != 0);
  out->u.o = new_viewAt(v, out->o, row, out->c);
  return Vt_obj;
}
static void free_viewRefs(View** vp, int n) {
  int i;
  for (i = 0; i < n; ++i)
    if (vp[i] != 0)
      view_decRef(vp[i]);
  ckfree((char*) vp);
}
static void view_cleaner(Column* c) {
  free_viewRefs((View**) c->ptr, c->count);
  std_cleaner(c);
}
static Column* make_viewCol(Tcl_Obj* list) {
  int e, i, ac;
  Tcl_Obj** av;
  Column* c;
  Buffer buf;
  Shared* sh = get_shared();
  e = Tcl_ListObjGetElements(0, list, &ac, &av); A(e == TCL_OK);
  buf_init(&buf);
  for (i = 0; i < ac; ++i) {
    View* v = av[i] != sh->none ? as_view(av[i]) : 0;
    BUF_ADD_PTR(buf, v != 0 ? view_incRef(v) : 0);
  }
  c = MAKE_COL(Column, get_shared()->none, buf_asPtr(&buf, 0), ac, g_view);
  c->cleaner = view_cleaner;
  return c;
}

static View* make_view(View* meta, int vsize) {
  Shared* sh = get_shared();
  int i, nc = meta != 0 ? view_size(meta) : 3;
  View* v = (View*) ckalloc(sizeof(View) + nc * sizeof(Colref));
  v->meta = meta != 0 ? view_incRef(meta) : 0;
  v->sizer = std_sizer;
  v->refs = 0;
  v->size = vsize;
  v->ip = sh->cip;
  for (i = 0; i <= nc; ++i) /* there is always at least one entry */
    v->cols[i].c = 0;
  return view_incRef(v); /* FIXME: refcount should not be inc'ed here */
}
static void view_setCol(View* v, int cn, Colref cr) {
  col_incRef(cr.c);
  col_decRef(v->cols[cn].c);
  v->cols[cn] = cr;
}
static View* make_indirView(View* meta, Column* c) {
  Colref cr;
  int nc = view_size(meta);
  View* v = make_view(meta, c->count);
  cr.c = c;
  cr.i = 0;
  /* always store at least one column reference, even if nc is zero */
  do view_setCol(v, cr.i, cr); while (++cr.i < nc);
  return v;
}

static Vtype g_cmd(Column* cv, int row, Value *vp) {
  int e, i, ac, col = vp->c;
  Tcl_SavedResult save;
  Shared* sh = get_shared();
  Tcl_Obj **av, *vop, *buf[10], **cvec = buf;
  Tcl_SaveResult(sh->cip, &save);
  e = Tcl_ListObjGetElements(0, cv->aux, &ac, &av); A(e == TCL_OK);
  if (ac > 7)
    cvec = (Tcl_Obj**) ckalloc((ac + 3) * sizeof(Tcl_Obj*));
  vop = Tcl_ObjGetVar2(sh->cip, sh->vops, *av, TCL_GLOBAL_ONLY); A(vop != 0);
  cvec[0] = l_index(vop, row < 0 ? 1 : 2); A(cvec[0] != 0);
  cvec[1] = cv->aux;
  for (i = 1; i < ac; ++i)
    cvec[i+1] = av[i];
  if (row < 0)
    e = Tcl_EvalObjv(sh->cip, ac + 1, cvec, TCL_EVAL_GLOBAL);
  else {
    cvec[ac+1] = incr_ref(new_int(row));
    cvec[ac+2] = incr_ref(new_int(col));
    e = Tcl_EvalObjv(sh->cip, ac + 3, cvec, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cvec[ac+1]);
    Tcl_DecrRefCount(cvec[ac+2]);
  }
  if (cvec != buf)
    ckfree((char*) cvec);
  D if (e != TCL_OK) printf("cmd_evalCmd? %s\n", Tcl_GetStringResult(sh->cip));
  A(e == TCL_OK);
  vp->u.o = incr_ref(Tcl_GetObjResult(sh->cip));
  Tcl_RestoreResult(sh->cip, &save);
  --vp->u.o->refCount; /* drop refcount, possibly to 0, but do not free obj */
  return Vt_obj;
}
static int cmd_sizer(View* v) {
  Value vb;
  vb.c = -1;
  return val_toInt(g_cmd(v->cols[0].c, -1, &vb), &vb);
}
static View* make_cmdView(Tcl_Obj* cmd) {
  Column *c = MAKE_COL(Column, cmd, 0, -1, g_cmd);
  View* v = make_indirView(as_view(l_index(cmd, 1)), c);
  v->sizer = (Sizer) cmd_sizer;
  return v;
}

static View* as_view(Tcl_Obj* obj) {
  if (obj->typePtr != &view_type)
    if (Tcl_ConvertToType(0, obj, &view_type) != TCL_OK)
      return 0;
  return VIEW_REF(obj);
}

static void freeViewIntRep(Tcl_Obj *o) {
  View* v = VIEW_REF(o);
  Origin* d = VIEW_ORG(o);
  Shared* sh = get_shared();
  Tcl_Interp* oip = sh->cip;
  /* either there is a string rep or the object is being free'd */
// A(o->bytes != 0 || o->refCount <= 0);
  /* cip needs to be set to support cleanup of var traces, see ref_vop */
  sh->cip = v->ip;
  view_decRef(v);
  sh->cip = oip;
  Tcl_DecrRefCount(d->def);
  if (d->deps != 0) {
    drop_deps(o);    
    Tcl_DecrRefCount(d->deps);
  }
  ckfree((char*) d);
}
static void updateViewStrRep(Tcl_Obj *o) {
  Tcl_Obj* g = VIEW_ORG(o)->def;
  A(g != get_shared()->none);
  Tcl_GetString(g);
  A(o->bytes == 0);
  A(g->bytes != 0);
  o->length = g->length; g->length = 0;
  o->bytes = g->bytes; g->bytes = 0;
}
static int setViewFromAnyRep(Tcl_Interp *ip, Tcl_Obj *o) {
  View* v;
  int ac;
  Tcl_Obj **av, *p, *deps = 0;
  Origin* d = (Origin*) ckalloc(sizeof(Origin));
  Shared* sh = get_shared();
  A(o != sh->none && o->typePtr != &view_type);
  if (Tcl_ListObjGetElements(ip, o, &ac, &av) != TCL_OK)
    return TCL_ERROR;
  switch (ac) {
    case 0:
      v = sh->zmeta;
      break;
    case 1: {
      int nrows;
      if (Tcl_GetIntFromObj(ip, *av, &nrows) != TCL_OK)
        return TCL_ERROR;
      v = make_view(sh->zmeta, nrows);
      break;
    }
    default: {
      int e, i;
      Tcl_SavedResult save;
      Tcl_Obj *t, *vop, *buf[10], **cvec = buf;
      A(sh->cip != 0);
      Tcl_SaveResult(sh->cip, &save);
      if (ac > 9)
        cvec = (Tcl_Obj**) ckalloc(ac * sizeof(Tcl_Obj*));
      vop = Tcl_ObjGetVar2(sh->cip, sh->vops, *av, TCL_GLOBAL_ONLY);
      if (vop == 0) {
        Tcl_SetObjResult(sh->cip, *av);
        Tcl_AppendResult(sh->cip, ": view operator gone?", 0);
        e = TCL_ERROR;
      } else {
        cvec[0] = l_index(vop, 0); A(cvec[0] != 0);
        for (i = 1; i < ac; ++i)
          cvec[i] = av[i];
        e = Tcl_EvalObjv(sh->cip, ac, cvec, 0);
      }
      if (ac > 9)
        ckfree((char*) cvec);
      if (e == TCL_OK) {
        t = Tcl_GetObjResult(sh->cip);
        if (l_length(vop) < 3) {
          v = as_view(t);
          if (v == 0)
            e = TCL_ERROR;
        } else
          v = make_cmdView(t);
          if (e == TCL_OK && t->typePtr == &view_type &&
              VIEW_ORG(t)->deps != 0)
            deps = incr_ref(t);
      }
      if (e != TCL_OK) {
        Tcl_DiscardResult(&save);
        return TCL_ERROR;
      }
      Tcl_RestoreResult(sh->cip, &save);
    }
  }
  p = Tcl_NewListObj(ac, av);
  if (o->typePtr != 0 && o->typePtr->freeIntRepProc != 0)
    o->typePtr->freeIntRepProc(o);
  d->def = incr_ref(p);
  d->deps = 0;
  VIEW_REF(o) = view_incRef(v);
  VIEW_ORG(o) = d;
  o->typePtr = &view_type;
  if (deps != 0) {
    add_deps(deps, o);
    Tcl_DecrRefCount(deps);
  }
  return TCL_OK;
}

static Tcl_ObjType view_type = {
  "view", freeViewIntRep, dupFakeIntRep, updateViewStrRep, setViewFromAnyRep
};

static Tcl_Obj* view_col(Tcl_Obj* view, View* v, int col) {
  int i, n = view_size(v);
  if (col >= 0) {
    Vtype t;
    Value vb;
    Buffer buf;
    Column* c = v->cols[col].c;
    vb.c = v->cols[col].i;
    vb.o = view;
    buf_init(&buf);
    for (i = 0; i < n; ++i) {
      t = c->getter(c, i, &vb);
      BUF_ADD_PTR(buf, t == Vt_obj ? vb.u.o : val_toObj(t, &vb));
    }
    return buf_asList(&buf);
  }
  return new_col(MAKE_COL(Column, Tcl_NewObj(), 0, n, g_iota), -1);
}
static Tcl_Obj* view_rows(Tcl_Obj* view, int row, int nr, int tags) {
  View* v = as_view(view);
  int i, j, nc = view_width(v);
  Buffer buf;
  buf_init(&buf);
  for (i = 0; i < nr; ++i)
    for (j = 0; j < nc; ++j) {
      if (tags)
        BUF_ADD_PTR(buf, view_at(0, v->meta, j, 0));
      BUF_ADD_PTR(buf, view_at(view, v, row+i, j));
    }
  return buf_asList(&buf);
}

static Tcl_Obj* desc_view(const char** ptr, const char* end) {
  Tcl_Obj *name, *meta = Tcl_NewObj();
  const char* p = *ptr;
  while (p < end) {
    const char* s = p;
    while (p < end && strchr(",[]", *p) == 0)
      ++p;
    name = Tcl_NewStringObj(s, p-s);
    if (*p == '[') {
      ++p;
      l_append(meta, new_list4(name, desc_view(&p, end), 0, 0));
      A(*p == ']');
      ++p;
    } else
      l_append(meta, name);
    if (p >= end || *p != ',')
      break;
    ++p;
  }
  *ptr = p;
  return meta;
}
static int reversed_endian(Map* m) {
  A(m->len > 0);
#if _BIG_ENDIAN
  return *m->ptr == 'J';
#else
  return *m->ptr == 'L';
#endif
}
static Getter fix_width(int size, int rows, int real, int flip) {
  static char widths[8][7] = {
    {0,-1,-1,-1,-1,-1,-1},
    {0, 8,16, 1,32, 2, 4},
    {0, 4, 8, 1,16, 2,-1},
    {0, 2, 4, 8, 1,-1,16},
    {0, 2, 4,-1, 8, 1,-1},
    {0, 1, 2, 4,-1, 8,-1},
    {0, 1, 2, 4,-1,-1, 8},
    {0, 1, 2,-1, 4,-1,-1},
  };
  int w = rows < 8 && size < 7 ? widths[rows][size] : (size << 3) / rows;
  switch (w) {
    case 0:   return g_i0;
    case 1:   return g_i1;
    case 2:   return g_i2;
    case 4:   return g_i4;
    case 8:   return g_i8;
    case 16:  return flip ? g_i16r : g_i16;
    case 32:  return real ? flip ? g_f32r : g_f32 : flip ? g_i32r : g_i32;
    case 64:  return real ? flip ? g_f64r : g_f64 : flip ? g_i64r : g_i64;
  }
  A(0);
  return g_i0;
}
static int fix_fudge(int n, int w) {
  static char fudges[3][4] = {  /* n:  1:  2:  3:  4: */
    {3,3,4,5},      /* 1-bit entries:  1b  2b  3b  4b */
    {5,5,1,1},      /* 2-bit entries:  2b  4b  6b  8b */
    {6,1,2,2},      /* 4-bit entries:  4b  8b 12b 16b */
  };
  return n <= 4 && w <= 3 ? fudges[w-1][n-1] : ((n<<w)+14)>>4;
}
static Column* fix_col(Tcl_Obj* map, int rows, char** c, int real) {
  int csize = get_v(c);
  int cpos = csize ? get_v(c) : 0;
  Map* m = as_map(map);
  Getter getter = fix_width(csize, rows, real, reversed_endian(m));
  return MAKE_COL(Column, map, m->ptr + cpos, rows, getter);
}

  typedef struct VarColumn {
    Column c;
    Map* mp;
    int istext, *offs, *memop;
    Column* sizes;
  } VarColumn;

static Vtype g_var(VarColumn* vc, int row, Value* vp) {
  int n = 0;
  int off = vc->offs[row];
  if (off > 0) {
    Value vb;
#ifndef NDEBUG
    Vtype t =
#endif
      vc->sizes->getter(vc->sizes, row, &vb);
      A(t == Vt_int);
    n = vb.u.i;
  }
  else if (off < 0) {
    n = vc->memop[~off];
    off = vc->memop[~off+1];
  }
  n -= vc->istext;
  if (n > 0) {
    vp->n = n;
    vp->u.p = vc->mp->ptr + off;
    if (vc->istext) {
      vp->o = vc->c.aux;
      return Vt_lzy;
    }
    return Vt_bin;
  }
  vp->n = 0;
  return Vt_nil;
}
static Column* var_col(Tcl_Obj* map, int rows, char** c, int istext) {
  char* cmem;
  char* limit;
  int i, *offs;
  VarColumn* vc;
  Buffer buf;
  Tcl_Obj *aux, *vec = Tcl_NewObj();
  int csize = get_v(c);
  int cpos = csize ? get_v(c) : 0;
  Map* m = as_map(map);
  Column* sizes = csize ? fix_col(map, rows, c, 0)
                        : MAKE_COL(Column, map, 0, rows, g_i0);
  csize = get_v(c);
  cmem = m->ptr + (csize ? get_v(c) : 0);
  offs = (int*) Tcl_SetByteArrayLength(vec, rows * sizeof(int));
  if (cpos != 0) {
    Vtype t;
    Value vb;
    for (i = 0; i < rows; ++i) {
      offs[i] = cpos;
      t = sizes->getter(sizes, i, &vb); A(t == Vt_int);
      cpos += vb.u.i;
    }
  } else
    memset(offs, 0, rows * sizeof(int));
  buf_init(&buf);
  limit = cmem + csize;
  i = 0;
  while (cmem < limit) {
    i += get_v(&cmem);
    offs[i++] = ~ (BUF_BYTES(buf) / sizeof(int));
    BUF_ADD_INT(buf, get_v(&cmem));
    BUF_ADD_INT(buf, get_v(&cmem));
  }
  aux = buf_asIntCol(&buf, 0);
  cmem = as_col(aux).c->ptr;
  aux = new_list4(map, vec, aux, new_col(sizes, -1));
  vc = MAKE_COL(VarColumn, aux, 0, rows, g_var);
  vc->mp = as_map(map);
  vc->offs = offs;
  vc->sizes = sizes;
  vc->memop = (int*) cmem;
  vc->istext = istext;
  return &vc->c;
}

  typedef struct SubColumn {
    Column c;
    View *meta, **ptrs;
    Tcl_Obj* map;
  } SubColumn;

static Vtype g_sub(SubColumn* sc, int row, Value* out) {
  if (sc->ptrs[row] == 0) {
    int off = ((const int*) sc->c.ptr)[row];
    sc->ptrs[row] = view_incRef(sub_view(sc->map, off, sc->meta));
  }
  out->u.o = new_viewAt(sc->ptrs[row], out->o, row, out->c);
  return Vt_obj;
}
static void sub_cleaner(Column* c) {
  free_viewRefs(((SubColumn*) c)->ptrs, c->count);
  std_cleaner(c);
}
static Column* sub_col(Tcl_Obj* map, int rows, char** c, View* mv) {
  /* TODO: combine stubs and offsets into a single array to halve mem use */
  int e, i, j, *offs;
  Tcl_Obj* aux;
  Column *offsets;
  SubColumn *sc;
  int csize = get_v(c);
  int cpos = csize ? get_v(c) : 0;
  int cols = view_size(mv);
  int bytes = rows * sizeof(View*);
  Map* m = as_map(map);
  char* cs = m->ptr + cpos;
  const char* types = Tcl_GetString(view_col(0, mv, 1));
  /* Set up an array of offsets, used later by the "g_sub" function.
     Allows indexed subview access, even for the current 2.4 file format. */
  offs = make_intCol(rows, &offsets);
  /* "map" acts as a tag which can be recognized by get_sub */
  aux = new_list4(map, new_view(mv, 0), new_col(offsets, -1), 0);
  for (i = 0; i < rows; ++i) {
    offs[i] = cs - m->ptr;
    e = get_v(&cs); A(e == 0);
    /* FIXME: doesn't handle desc string in subviews */
    if (get_v(&cs) > 0)
      for (j = 0; j < cols; ++j) {
        switch (types[2*j]) { case 'B': case 'S': if (get_p(&cs)) get_p(&cs); }
        get_p(&cs);
      }
  }
  sc = MAKE_COL(SubColumn, aux, (char*) offs, rows, g_sub);
  sc->ptrs = (View**) memset(ckalloc(bytes), 0, bytes);
  sc->meta = mv;
  sc->map = map;
  sc->c.cleaner = sub_cleaner;
  return &sc->c;
}
static View* sub_view(Tcl_Obj* map, int off, View* mv) {
  int cols, rows, i;
  View* v;
  Colref cr;
  Map* m = as_map(map);
  char* cs = m->ptr + off;
  i = get_v(&cs); A(i == 0); /* skip */
  if (mv == 0) {
    int desclen = get_v(&cs);
    const char* p = cs;
    cs += desclen;
    mv = as_view(new_list4(get_shared()->mdef, desc_view(&p, cs), 0, 0));
    A(p == cs);
  }
  rows = get_v(&cs);
  v = make_view(mv, rows);
  cols = view_size(mv);
  if (rows > 0)
    for (cr.i = 0; cr.i < cols; ++cr.i) {
      switch (*Tcl_GetString(view_at(0, mv, cr.i, 1))) {
        case 'S': cr.c = var_col(map, rows, &cs, 1); break;
        case 'B': cr.c = var_col(map, rows, &cs, 0); break;
        case 'V': cr.c = sub_col(map, rows, &cs,
                                  as_view(view_at(0, mv, cr.i, 2)));
                  break;
        case 'D': case 'F': cr.c = fix_col(map, rows, &cs, 1); break;
        default:  cr.c = fix_col(map, rows, &cs, 0); break;
      }
      view_setCol(v, cr.i, cr);
    }
  else if (cols > 0) {
    /* TODO: could keep a shared dummy col in Shared */
    cr.c = MAKE_COL(Column, Tcl_NewObj(), 0, 0, g_i0);
    for (cr.i = 0; cr.i < cols; ++cr.i)
      view_setCol(v, cr.i, cr);
  }
  return v;
}

/* - - -  COMMANDS  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int view_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int e = TCL_OK, k = 0;

  if (oc < 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "v ... | ...");
    return TCL_ERROR;
  }

  Tcl_SetObjResult(sh->cip, ov[1]); oc -= 2; ov += 2;

  while (e == TCL_OK && oc > 0) {
    int i, n;
    for (n = 0; n < oc; ++n)
      if (ov[n]->bytes != 0 && ov[n]->bytes[0] == '|' && ov[n]->bytes[1] == 0)
        break;
    if (n > 0) {
      Tcl_Obj *vop, *buf[10], **cvec = buf;
      if (n > 9)
        cvec = (Tcl_Obj**) ckalloc((n + 1) * sizeof(Tcl_Obj*));
      vop = Tcl_ObjGetVar2(sh->cip, sh->vops, *ov, TCL_GLOBAL_ONLY);
      if (vop == 0) {
        Tcl_SetObjResult(sh->cip, *ov);
        Tcl_AppendResult(sh->cip, ": no such view operation", 0);
        e = TCL_ERROR;
      } else {
        A(l_length(vop) > 0);
        cvec[1] = incr_ref(Tcl_GetObjResult(sh->cip));
        for (i = 1; i < n; ++i)
          cvec[i+1] = ov[i];
        if (l_length(vop) == 1) {
          cvec[0] = l_index(vop, 0); A(cvec[0] != 0);
          e = Tcl_EvalObjv(sh->cip, n+1, cvec, 0);
        } else {
          cvec[0] = *ov;
          Tcl_SetObjResult(sh->cip, Tcl_NewListObj(n+1, cvec));
        }
        Tcl_DecrRefCount(cvec[1]);
      }
      if (n > 9)
        ckfree((char*) cvec);
    }
    oc -= n+1; ov += n+1; ++k;
  }

  if (e != TCL_OK) {
    char msg[50];
    sprintf(msg, "\n    (\"view\" step %d)", k);
    Tcl_AddObjErrorInfo(sh->cip, msg, -1);
  }
  return e;
}

static int col_num(Tcl_Interp* ip, View* m, Tcl_Obj* obj) {
  int col;
  switch (*Tcl_GetString(obj)) {
    case '#': return -2;
    case '*': return -1;
    case '-': case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':
      if (Tcl_GetIntFromObj(ip, obj, &col) != TCL_OK)
        return -3;
      if (col < 0)
        col += view_size(m);
      break;
    default: {
      /* TODO: messy, make sure column has been converted to a g_svec */
      Colref cr;
      cr = m->cols[0];
      if (cr.c->getter != g_svec) {
        col_convert(&cr, g_svec, 0, 1);
        view_setCol(m, 0, cr);
      }
      if (Tcl_GetIndexFromObj(ip, obj, (const char**) cr.c->ptr,
                                          "column", 0, &col) != TCL_OK) {
        D printf("vget col?\n");
        return -3;
      }
    }
  }
  return col;
}
static int colnum_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  View *v;
  int i, n, nc;
  Tcl_Obj* map = Tcl_NewObj();

  Tcl_SetObjResult(sh->cip, map);
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view colnums");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0 ||
      Tcl_ListObjLength(sh->cip, ov[2], &n) != TCL_OK)
    return TCL_ERROR;
  nc = view_width(v);
  if (n > 0 && strcmp(Tcl_GetString(l_index(ov[2], 0)), "-omit") == 0) {
    Buffer buf;
    u_char* omit = Tcl_SetByteArrayLength(map, nc);
    memset(omit, 1, nc);
    for (i = 1; i < n; ++i) {
      int col = col_num(sh->cip, v->meta, l_index(ov[2], i));
      if (col < -2)
        return TCL_ERROR;
      omit[col] = 0;
    }
    buf_init(&buf);
    for (i = 0; i < nc; ++i)
      if (omit[i])
        BUF_ADD_INT(buf, i);
    Tcl_SetObjResult(sh->cip, buf_asIntCol(&buf, nc));
  } else
    for (i = 0; i < n; ++i) {
      int col = col_num(sh->cip, v->meta, l_index(ov[2], i));
      if (col < -2)
        return TCL_ERROR;
      l_append(map, new_int(col));      
    }
  return TCL_OK;
}

static int vget_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  Tcl_Obj* vobj;
  View *v;

  if (oc < 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view ...");
    return TCL_ERROR;
  }

  vobj = ov[1]; oc -= 2; ov += 2;

  if (oc == 0) {
    if ((v = as_view(vobj)) == 0)
      return TCL_ERROR;
    vobj = view_rows(vobj, 0, view_size(v), 0);
  } else {
    int i;
    for (i = 0; i < oc; ++i) {
      int row, col, nr;
      Tcl_SetObjResult(sh->cip, vobj);

      if ((v = as_view(vobj)) == 0)
        return TCL_ERROR;
      nr = view_size(v);

      if (Tcl_GetIntFromObj(0, ov[i], &row) != TCL_OK)
        switch (*Tcl_GetString(ov[i])) {
          case '@': vobj = new_view(v->meta, new_list4(sh->meta, vobj, 0, 0));
                    continue;
          case '#': vobj = new_int(nr); continue;
          case '*': row = -1; break;
          default:  Tcl_GetIntFromObj(sh->cip, ov[i], &row); return TCL_ERROR;
        }
      else if (row < 0)
        row += nr;

      if (++i >= oc) {
        if (row >= 0)
          vobj = view_rows(vobj, row, 1, 1); /* one tagged row */
        else {
          Buffer buf;
          buf_init(&buf);
          for (i = 0; i < nr; ++i)
            BUF_ADD_PTR(buf, view_rows(vobj, i, 1, 0)); /* list of rows */
          vobj = buf_asList(&buf);
        }
        break;
      }

      col = col_num(sh->cip, v->meta, ov[i]);

      if (row >= 0)
        switch (col) {
          case -1:  vobj = view_rows(vobj, row, 1, 0); break;
          case -2:  vobj = new_int(row); break;
          case -3:  return TCL_ERROR;
          default:  vobj = view_at(vobj, v, row, col); break;
        }
      else
        switch (col) {
          case -1:  vobj = view_rows(vobj, 0, nr, 0); break;
          case -2:  vobj = view_col(vobj, v, -1); break;
          case -3:  return TCL_ERROR;
          default:  vobj = view_col(vobj, v, col); break;
        }
    }
    A(vobj != 0);
  }
  Tcl_SetObjResult(sh->cip, vobj);
  return TCL_OK;
}

  typedef struct LoopInfo {
    int row;
    Tcl_Obj* view;
    View* v;
    Tcl_Obj* cursor;
  } LoopInfo;

  typedef struct ColTrace {
    LoopInfo* lip;
    int col;
    Tcl_Obj* name;
    int lastRow;
  } ColTrace;

static char* cursor_tracer(ClientData cd, Tcl_Interp* ip, const char* name1, const char* name2, int flags) {
  ColTrace* ct = (ColTrace*) cd;
  int row = ct->lip->row;
  if (row != ct->lastRow) {
    Tcl_Obj* o = ct->col < 0 ? new_int(row) :
                      view_at(ct->lip->view, ct->lip->v, row, ct->col);
    if (Tcl_ObjSetVar2(ip, ct->lip->cursor, ct->name, o, 0) == 0)
      return "cursor_tracer?";
    ct->lastRow = row;
  }
  return 0;
}

static int loop_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  LoopInfo li;
  const char* cur;
  Buffer result;
  int type = -1, e = TCL_OK;
  static const char* options[] = { "-where", "-index", "-collect", 0 };
  enum { eLOOP = -1, eWHERE, eINDEX, eCOLLECT };

  if (oc < 3 || oc > 5) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view ?arrayName? ?-type? body");
    return TCL_ERROR;
  }

  Tcl_SetObjResult(sh->cip, ov[1]); oc -= 2; ov += 2;
  if (oc > 1 && *Tcl_GetString(*ov) != '-') {
    li.cursor = *ov++; --oc;
  } else
    li.cursor = Tcl_NewObj();
  cur = Tcl_GetString(incr_ref(li.cursor));

  if (Tcl_GetIndexFromObj(0, *ov, options, "", 0, &type) == TCL_OK) {
    --oc; ++ov;
  }

  li.view = incr_ref(Tcl_GetObjResult(sh->cip));
  if ((li.v = as_view(li.view)) == 0)
    e = TCL_ERROR;
  else {
    int i, nr = view_size(li.v), nc = view_width(li.v);
    ColTrace* ctab = (ColTrace*) ckalloc((nc + 1) * sizeof(ColTrace));
    buf_init(&result);
    for (i = 0; i <= nc; ++i) {
      ColTrace* ctp = ctab + i;
      ctp->lip = &li;
      ctp->lastRow = -1;
      if (i < nc) {
        ctp->col = i;
        ctp->name = view_at(0, li.v->meta, i, 0);
      } else {
        ctp->col = -1;
        ctp->name = sh->hash;
      }
      e = Tcl_TraceVar2(sh->cip, cur, Tcl_GetString(ctp->name),
                          TCL_TRACE_READS, cursor_tracer, (ClientData) ctp);
    }
    if (e == TCL_OK)
      switch (type) {
        case eLOOP:
          for (li.row = 0; li.row < nr; ++li.row) {
            A((*ov)->typePtr != &view_type);
            e = Tcl_EvalObj(sh->cip, *ov);
            if (e == TCL_CONTINUE)
              e = TCL_OK;
            else if (e != TCL_OK) {
              if (e == TCL_BREAK)
                e = TCL_OK;
              else if (e == TCL_ERROR) {
                char msg[50];
                sprintf(msg, "\n  (\"loop\" body line %d)",
                                        sh->cip->errorLine);
                Tcl_AddObjErrorInfo(sh->cip, msg, -1);
              }
              break;
            }
          }
          break;
        case eWHERE: case eINDEX:
          for (li.row = 0; li.row < nr; ++li.row) {
            int f;
            e = Tcl_ExprBooleanObj(sh->cip, *ov, &f);
            if (e != TCL_OK)
              break;
            if (f)
              BUF_ADD_INT(result, li.row);
          }
          break;
        case eCOLLECT:
          for (li.row = 0; li.row < nr; ++li.row) {
            Tcl_Obj* o;
            e = Tcl_ExprObj(sh->cip, *ov, &o);
            if (e != TCL_OK)
              break;
            BUF_ADD_PTR(result, o);
          }
          break;
        default: A(0);
      }
    for (i = 0; i <= nc; ++i) {
      ColTrace* ctp = ctab + i;
      Tcl_UntraceVar2(sh->cip, cur, Tcl_GetString(ctp->name),
                          TCL_TRACE_READS, cursor_tracer, (ClientData) ctp);
    }
    ckfree((char*) ctab);
    if (e == TCL_OK)
      switch (type) {
        case eWHERE: case eINDEX: {
          Tcl_Obj *o = buf_asIntCol(&result, nr);
          if (type == eWHERE)
            o = new_list4(sh->remap, li.view, o, 0);
          Tcl_SetObjResult(sh->cip, o);
          break;
        }
        case eCOLLECT: {
          Tcl_Obj** rp = (Tcl_Obj**) buf_asPtr(&result, 1);
          int n = BUF_BYTES(result) / sizeof(Tcl_Obj*);
          Tcl_SetObjResult(sh->cip, Tcl_NewListObj(n, rp));
          for (i = 0; i < n; ++i)
            Tcl_DecrRefCount(rp[i]);
          break;
        }
      }
    buf_free(&result);
  }
  Tcl_DecrRefCount(li.view);
  Tcl_DecrRefCount(li.cursor);
  return e;
}

static Tcl_Obj* meta_view(Tcl_Obj* defs) {
  View *v;
  int i, nr;
  Shared* sh = get_shared();
  Tcl_Obj* names = Tcl_NewObj();
  Tcl_Obj* types = Tcl_NewObj();
  Tcl_Obj* subvs = Tcl_NewObj();
  A(defs != sh->none);
  /* messy: need to transpose row descriptions to columns */
  nr = l_length(defs);
  for (i = 0; i < nr; ++i) {
    Tcl_Obj* d = l_index(defs, i);
    int nc = l_length(d);
    Tcl_Obj* n = l_index(d, 0);
    Tcl_Obj* u = l_index(d, 1);
    Tcl_Obj* t = nc > 1 ? sh->vtype : sh->stype;
    const char* s = nc > 0 ? Tcl_GetString(n) : "?:S";
    const char* q = strchr(s, ':');
    if (q != 0) {
      l_append(names, Tcl_NewStringObj(s, q-s));
      if (*++q != 0)
        t = Tcl_NewStringObj(q, -1);
    } else
      l_append(names, n);
    l_append(types, t);
    l_append(subvs, nc > 1 ? meta_view(u) : Tcl_NewObj());
  }
  v = make_view(sh->mmeta, nr);
  view_setCol(v, 0, as_col(names));
  view_setCol(v, 1, as_col(types));
  view_setCol(v, 2, as_col(subvs));
  return new_view(v, 0);
}
static int mdef_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  if (oc != 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "desc");
    return TCL_ERROR;
  }
  Tcl_SetObjResult(sh->cip, meta_view(ov[1]));
  return TCL_OK;
}

static int vdata_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  View *v, *m;
  Colref cr;
  Tcl_Obj **av, *o = 0;
  int i, ac, flat = 0, nr = 0, nc = -1, off = 0, step = 1;
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "m d");
    return TCL_ERROR;
  }
  if (Tcl_ListObjGetElements(sh->cip, ov[2], &ac, &av) != TCL_OK)
    return TCL_ERROR;
/* FIXME: special-case to catch empty string input and use mmeta instead */
if (ov[1]->bytes != 0 && ov[1]->length == 0) m = sh->mmeta; else
  m = as_view(ov[1]); A(m != 0);
  nc = view_size(m);
  if (ac == 1 && nc > 1) { /* convert flat to column-wise */
    flat = 1;
    step = nc;
    o = *av;
    nr = as_col(o).c->count / nc;
  } else {
    A(ac <= nc);
    if (ac > 0)
      nr = as_col(*av).c->count;
  }
  v = make_view(m, nr);
  for (i = 0; i < nc; ++i) {
    if (flat)
      off = i;
    else
      o = i < ac ? av[i] : Tcl_NewObj();
    cr = as_col(o);
    if (nr > 0) {
      Getter g = off != 0 || step != 1 ? g_obj : 0;
      char type = *Tcl_GetString(view_at(0, m, i, 1));
      if (cr.c->getter == g_obj) 
        switch (type) {
          case 'I': g = g_i32; break;
#if 0
          case 'S': g = g_svec; break;
#endif
        }
      if (g != 0)
        col_convert(&cr, g, off, step);
      if (type == 'V' && cr.c->getter == g_obj) {
        cr.c = make_viewCol(cr.c->aux); /* FIXME: leaks mem */
        cr.i = i;
      }
    }
    view_setCol(v, i, cr);
  }
  Tcl_SetObjResult(sh->cip, new_view(v, 0));
  return TCL_OK;
}

static int mapf_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  if (oc != 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "datasrc");
    return TCL_ERROR;
  }
  Tcl_Obj* map = Tcl_NewListObj(oc, ov);
  Map* m = as_map(map);
  int off = int_32be((const u_char*) m->ptr + m->len - 4);
//Tcl_Obj* o = Tcl_NewListObj(oc, ov); /* no: ov[0] is not vop but full cmd */
  Tcl_Obj* o = new_list4(sh->mapf, ov[1], 0, 0);
  Tcl_SetObjResult(sh->cip, new_view(sub_view(map, off, 0), o));
  return TCL_OK;
}

  typedef struct RemapColumn {
    Column c;
    View* w;
    int start;
  } RemapColumn;

static Vtype g_remap(RemapColumn* ri, int row, Value* vp) {
  const int* map = (const int*) ri->c.ptr;
  int i;
  A(0 <= ri->start+row && ri->start+row < ri->c.count);
  i = map[ri->start+row];
  if (i < 0) {
    A(ri->start+row+i >= 0);
    i = map[ri->start+row+i];
  }
  return view_get(ri->w, i, vp);
}
static View* make_remapView(View* v, Colref cr, int start, int count) {
  RemapColumn* c;
  View* w;
  Tcl_Obj* aux;
  if (cr.c->getter != g_i32)
    col_convert(&cr, g_i32, 0, 1);
  aux = new_list4(new_view(v, 0), new_col(cr.c, cr.i), 0, 0);
  c = MAKE_COL(RemapColumn, aux, cr.c->ptr, cr.c->count, g_remap);
  w = make_indirView(v->meta, &c->c);
  w->size = count;
  c->w = v;
  c->start = start;
  return w;
}
static int remap_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  Colref cr;
  int count, start = 0;
  View* v;
  if (oc != 3 && oc != 5) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view map ?start count?");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  cr = as_col(ov[2]);
  count = cr.c->count;
  if (oc == 5 && (Tcl_GetIntFromObj(sh->cip, ov[3], &start) != TCL_OK ||
                  Tcl_GetIntFromObj(sh->cip, ov[4], &count) != TCL_OK))
    return TCL_ERROR;
  Tcl_SetObjResult(sh->cip, new_view(make_remapView(v, cr, start, count), 0));
  return TCL_OK;
}

  typedef struct CremapView {
    View* w;
    int* vec;
  } CremapView;

static Tcl_Obj* make_cremapView(View* v, Colref map) {
  Column* mc;
  View *w, *mv;
  int i, *imap;
  if (map.c->getter != g_i32)
    col_convert(&map, g_i32, 0, 1);
  mc = map.c;
  mv = make_remapView(v->meta, map, 0, mc->count);
  imap = (int*) mc->ptr;
  w = make_view(mv, view_size(v));
  for (i = 0; i < mc->count; ++i)
    view_setCol(w, i, v->cols[imap[i]]);
  return new_view(w, 0);
}
static int cremap_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  View* v;
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view map");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  Tcl_SetObjResult(sh->cip, make_cremapView(v, as_col(ov[2])));
  return TCL_OK;
}

  typedef struct StepColumn {
    Column c;
    View* v;
    int o, n, r, s;
  } StepColumn;

static Vtype g_step(StepColumn* vsi, int row, Value* vp) {
  int n = view_size(vsi->v);
  if (n <= 0) {
    vp->n = 0;
    return Vt_nil;
  }
  row = (vsi->o + (row / vsi->r) * vsi->s) % n;
  return view_get(vsi->v, row, vp);
}
static Tcl_Obj* make_stepView(View* v, int o, int n, int r, int s) {
  StepColumn* vsi = MAKE_COL(StepColumn, new_view(v, 0), 0, n * r, g_step);
  vsi->v = v;
  vsi->o = o;
  vsi->n = n;
  vsi->r = r;
  vsi->s = s;
  return new_view(make_indirView(v->meta, &vsi->c), 0);
}
static int vstep_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  View* v;
  int o, n, r, s;
  if (oc != 6) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "v o n r s");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  if (Tcl_GetIntFromObj(sh->cip, ov[2], &o) != TCL_OK ||
      Tcl_GetIntFromObj(sh->cip, ov[3], &n) != TCL_OK ||
      Tcl_GetIntFromObj(sh->cip, ov[4], &r) != TCL_OK ||
      Tcl_GetIntFromObj(sh->cip, ov[5], &s) != TCL_OK)
    return TCL_ERROR;
  Tcl_SetObjResult(sh->cip, make_stepView(v, o, n, r, s));
  return TCL_OK;
}

  typedef struct RefColumn {
    Column c;
    Tcl_Obj* var;
    View *v;
  } RefColumn;

static char* ref_tracer(RefColumn* rv, Tcl_Interp* ip, const char* name1, const char* name2, int flags) {
  View *v;
  Shared* sh = get_shared();
  Tcl_Interp* oip = sh->cip;
  Tcl_Obj* o = Tcl_ObjGetVar2(ip, rv->var, 0, TCL_GLOBAL_ONLY);
  A(o != 0);
  sh->cip = ip;
  v = as_view(o); A(v != 0);
  /* FIXME: width cannot change because owning view has fixed column slots */
//A(view_width(rv->v) == view_width(v));
  view_incRef(v);
//view_decRef(rv->v);
  rv->v = v;
  sh->cip = oip;
  return 0;
}
static int ref_sizer(View* v) {
  RefColumn* rv = (RefColumn*) v->cols[0].c;
  return view_size(rv->v);
}
static Vtype g_ref(RefColumn* rv, int row, Value* vp) {
  return view_get(rv->v, row, vp);
}
static void ref_cleaner(Column* c) {
  RefColumn* rv = (RefColumn*) c;
  Tcl_UntraceVar(get_shared()->cip, Tcl_GetString(rv->var),
                  TCL_TRACE_WRITES | TCL_GLOBAL_ONLY, 
                  (Tcl_VarTraceProc*) ref_tracer, (ClientData) rv);
  view_decRef(rv->v);
  std_cleaner(c);
}
static Tcl_Obj* make_refView(Tcl_Obj* var) {
  int e;
  View *w, *v;
  RefColumn* rv;
  Shared* sh = get_shared();
  Tcl_Obj *vw, *o = Tcl_ObjGetVar2(sh->cip, var, 0, TCL_GLOBAL_ONLY);
  A(o != 0);
  v = as_view(o); A(v != 0);
  rv = MAKE_COL(RefColumn, new_list4(var, o, 0, 0), 0, -1, g_ref);
  rv->var = var;
  rv->v = v;
  rv->c.cleaner = ref_cleaner;
  e = Tcl_TraceVar2(sh->cip, Tcl_GetString(var), 0,
                      TCL_TRACE_WRITES | TCL_GLOBAL_ONLY, 
                      (Tcl_VarTraceProc*) ref_tracer, (ClientData) rv);
  A(e == TCL_OK);
  w = make_indirView(v->meta, &rv->c);
  w->sizer = ref_sizer;
  vw = new_view(w, 0);
  A(VIEW_ORG(vw)->deps == 0);
  VIEW_ORG(vw)->deps = incr_ref(Tcl_NewListObj(1, &var));
  return vw;
}
static int ref_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  if (oc < 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "var ?cmd ...?");
    return TCL_ERROR;
  }
  Tcl_SetObjResult(sh->cip, make_refView(ov[1]));
  return TCL_OK;
}

static int at_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int rn, cn;
  View* v;
  Column* c;
  Value vb;
  if (oc != 4) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "v r c");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0 ||
      Tcl_GetIntFromObj(sh->cip, ov[2], &rn) != TCL_OK ||
      Tcl_GetIntFromObj(sh->cip, ov[3], &cn) != TCL_OK)
    return TCL_ERROR;
  c = v->cols[cn].c;
  vb.c = v->cols[cn].i;
  vb.o = ov[1];
  Tcl_SetObjResult(sh->cip, val_toObj(c->getter(c, rn, &vb), &vb));
  return TCL_OK;
}

static int value_cmp(Value* pa, Value* pb, char type, char order) {
  int e, f = 0;
  switch (type) {
    case 'I':
      e = val_toInt(pa->t, pa);
      f = val_toInt(pb->t, pb);
      return e == f ? 0 : e < f ? -1 : 1;
    case 'L':
      if (pa->t != Vt_wid) {
        e = Tcl_GetWideIntFromObj(0, val_toObj(pa->t, pa), &pa->u.q);
        A(e == TCL_OK);
        pa->t = Vt_wid;
      }
      if (pb->t != Vt_wid) {
        e = Tcl_GetWideIntFromObj(0, val_toObj(pb->t, pb), &pb->u.q);
        A(e == TCL_OK);
        pb->t = Vt_wid;
      }
      return pa->u.q == pb->u.q ? 0 : pa->u.q < pb->u.q ? -1 : 1;
    case 'F': case 'D':
      if (pa->t != Vt_dbl) {
        e = Tcl_GetDoubleFromObj(0, val_toObj(pa->t, pa), &pa->u.d);
        A(e == TCL_OK);
        pa->t = Vt_dbl;
      }
      if (pb->t != Vt_dbl) {
        e = Tcl_GetDoubleFromObj(0, val_toObj(pb->t, pb), &pb->u.d);
        A(e == TCL_OK);
        pb->t = Vt_dbl;
      }
      return pa->u.d == pb->u.d ? 0 : pa->u.d < pb->u.d ? -1 : 1;
    case 'S':
      if (pa->t != Vt_lzy && pa->t != Vt_str && pa->t != Vt_nil) {
        pa->u.p = Tcl_GetStringFromObj(val_toObj(pa->t, pa), &pa->n);
        pa->t = Vt_str;
      }
      if (pb->t != Vt_lzy && pb->t != Vt_str && pb->t != Vt_nil) {
        pb->u.p = Tcl_GetStringFromObj(val_toObj(pb->t, pb), &pb->n);
        pb->t = Vt_str;
      }
      if (pa->n == pb->n && memcmp(pa->u.p, pb->u.p, pa->n) == 0)
        return 0;
      if (order == '=') /* UTF8 is irrelevant when testing for equality */
        return 1;
      pa->n = Tcl_NumUtfChars(pa->u.p, pa->n);
      pb->n = Tcl_NumUtfChars(pb->u.p, pb->n);
      e = pa->n < pb->n ? pa->n : pb->n;
      f = 'a' <= order && order <= 'z' ? Tcl_UtfNcasecmp(pa->u.p, pb->u.p, e)
                                       : Tcl_UtfNcmp(pa->u.p, pb->u.p, e);
      break;
    case 'B':
      if (pa->t != Vt_bin && pa->t != Vt_nil) {
        pa->u.p = (char*) Tcl_GetByteArrayFromObj(val_toObj(pa->t, pa), &pa->n);
        pa->t = Vt_bin;
      }
      if (pb->t != Vt_bin && pb->t != Vt_nil) {
        pb->u.p = (char*) Tcl_GetByteArrayFromObj(val_toObj(pb->t, pb), &pb->n);
        pb->t = Vt_bin;
      }
      f = memcmp(pa->u.p, pb->u.p, pa->n < pb->n ? pa->n : pb->n);
      break;
    default: A(0);
  }
  if (f == 0 && pa->n != pb->n)
    f = pa->n < pb->n ? 1 : -1;
  return f;
}
static int less_than(View* v, const char* types, int nc, int a, int b) {
  if (a != b) {
    Value va, vb;
    int i, f;
    for (i = 0; i < nc; ++i) {
      va.c = vb.c = i;
      va.t = view_get(v, a, &va);
      vb.t = view_get(v, b, &vb);
      f = value_cmp(&va, &vb, types[2*i], 'A');
      if (f != 0)
        return f < 0;
    }
  }
  return a < b;
}
static int test_swap(View* v, const char* types, int nc, int* a, int* b) {
  if (less_than(v, types, nc, *b, *a)) {
    int t = *a; *a = *b; *b = t;
    return 1;
  }
  return 0;
}
static void merge_sort(View* v, const char* types, int nc, int* ar, int nr, int* scr) {
  A(nr > 1 && nc > 0);
  switch (nr) {
    case 2:
      test_swap(v, types, nc, ar, ar+1);
      break;
    case 3:
      test_swap(v, types, nc, ar, ar+1);
      if (test_swap(v, types, nc, ar+1, ar+2))
        test_swap(v, types, nc, ar, ar+1);
      break;
    case 4: /* TODO: optimize with if's */
      test_swap(v, types, nc, ar, ar+1);
      test_swap(v, types, nc, ar+2, ar+3);
      test_swap(v, types, nc, ar, ar+2);
      test_swap(v, types, nc, ar+1, ar+3);
      test_swap(v, types, nc, ar+1, ar+2);
      break;
      /* TODO: also special-case 5-item sort? */
    default: {
      int s1 = nr / 2, s2 = nr - s1;
      int *f1 = scr, *f2 = scr + s1, *t1 = f1 + s1, *t2 = f2 + s2;
      merge_sort(v, types, nc, f1, s1, ar);
      merge_sort(v, types, nc, f2, s2, ar+s1);
      for (;;)
        if (less_than(v, types, nc, *f1, *f2)) {
          *ar++ = *f1++;
          if (f1 >= t1) {
            while (f2 < t2) 
              *ar++ = *f2++;
            break;
          }
        } else {
          *ar++ = *f2++;
          if (f2 >= t2) {
            while (f1 < t1)
              *ar++ = *f1++;
            break;
          }
        }
    }
  }
}
static int sortmap_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int nr;
  View *v;
  if (oc != 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "v");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  nr = view_size(v);
  if (nr > 0) {
    Column *mapc, *tmpc;
    int i, nc = view_width(v);
    const char* types = Tcl_GetString(view_col(0, v->meta, 1));
    int *imap = make_intCol(nr, &mapc), *itmp = make_intCol(nr, &tmpc);
    for (i = 0; i < nr; ++i)
      imap[i] = i;
    if (nr > 1 && nc > 0) {
      memcpy(itmp, imap, nr * sizeof(int));
      merge_sort(v, types, nc, imap, nr, itmp);
    }
    col_decRef(tmpc);
    Tcl_SetObjResult(sh->cip, new_col(mapc, -1));
  }
  return TCL_OK;
}

static int find_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int i, nr;
  View *v;
  Value vb, *vals;
  const char* types;
  if (oc < 2) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view key ...");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  oc -= 2; ov += 2;
  if (oc != view_width(v)) {
    Tcl_SetResult(sh->cip, "argument count does not match column count",
                    TCL_STATIC);
    return TCL_ERROR;
  }
  types = Tcl_GetString(view_col(0, v->meta, 1));
  nr = view_size(v);
  vals = (Value*) ckalloc(oc * sizeof(Value));
  for (i = 0; i < oc; ++i) {
    vals[i].u.o = ov[i];
    vals[i].t = Vt_obj;
  }
  for (i = 0; i < nr; ++i) {
    for (vb.c = 0; vb.c < oc; ++vb.c) {
      vb.t = view_get(v, i, &vb);
      if (value_cmp(&vb, vals + vb.c, types[2*vb.c], '='))
        break;
    }
    if (vb.c == oc)
      break;
  }
  ckfree((char*) vals);
  Tcl_SetObjResult(sh->cip, new_int(i < nr ? i : -1));
  return TCL_OK;
}

static int ibytes_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  static Getter ftab[] = { g_i32, g_i32, g_i64, g_f32, g_f64 };
  int e, f, lo, hi, j, w, n, *v;
  Colref cr;
  Tcl_Obj* o = Tcl_NewObj();
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "c f");
    return TCL_ERROR;
  }
  e = Tcl_GetIntFromObj(0, ov[2], &f); A(e == TCL_OK);
  A(0 <= f && f <= 4);
  cr = as_col(ov[1]);
  col_convert(&cr, ftab[f], 0, 1);
  n = cr.c->count;
  v = (int*) cr.c->ptr;
  switch (f) {
    case 0: case 1:
      lo = hi = 0;
      for (j = 0; j < n; ++j) {
        if (v[j] < lo) lo = v[j];
        if (v[j] > hi) hi = v[j];
      }
      w = min_width(lo, hi);
      break;
    case 2: case 4:
      w = 7;
      break;
    case 3:
      w = 6;
      break;
    default:
      A(0);
  }
  if (f == 0) {
    l_append(o, new_int(lo));
    l_append(o, new_int(hi));
    l_append(o, new_int(top_bit((unsigned) (lo|hi))));
    l_append(o, new_int(w));
  } else if (w >= 6) {
    int m = n * (1 << (w-4));
    u_char* b = Tcl_SetByteArrayLength(o, m);
    A(w <= 7);
    memcpy(b, v, m);
  } else if (n > 0 && w > 0) {
    int f = fix_fudge(n, w);
    u_char* b = Tcl_SetByteArrayLength(o, f);
    int* e = v + n, i = 0;
    memset(b, 0, f);
    switch (w) {
      case 1: { /* 1 bit, 8 per byte */
        char* q = (char*) b;
        while (v < e) { *q |= (*v++&1) << i; ++i; q += i>>3; i &= 7; }
        break;
      }
      case 2: { /* 2 bits, 4 per byte */
        char* q = (char*) b;
        while (v < e) { *q |= (*v++&3) << i; i += 2; q += i>>3; i &= 7; }
        break;
      }
      case 3: { /* 4 bits, 2 per byte */
        char* q = (char*) b;
        while (v < e) { *q |= (*v++&15) << i; i += 4; q += i>>3; i &= 7; }
        break;
      }
      case 4: { /* 1-byte (char) */
        char* q = (char*) b;
        while (v < e) *q++ = (char)*v++;
        break;
      }
      case 5: { /* 2-byte (short) */
        short* q = (short*) b;
        while (v < e) *q++ = (short)*v++;
        break;
      }
      default: A(0);
    }
  }
  col_decRef(cr.c);
  Tcl_SetObjResult(sh->cip, o);
  return TCL_OK;
}

  typedef struct GroupInfo {
    Column *hvc, *hmc, *gmc;
    const char* types;
    int hmpri, nc, *hvp, *hmp, *gmp;
    View* v;  
  } GroupInfo;

static int hash_find(View* w, int* hwp, int row, GroupInfo* gip) {
  Value v1, v2;
  int i, j, hv = hwp[row];
  int z, mask = gip->hmc->count - 1, o = mask & (hv ^ (hv >> 3));
  if (o == 0) o = mask;
  i = mask & ~hv;
  for (;;) {
    i = mask & (i + o);
    z = gip->hmp[i];
    if (z < 0) break;
    if (hv == gip->hvp[gip->gmp[z]]) {
      for (j = 0; j < gip->nc; ++j) {
        v1.c = v2.c = j;
        v1.t = view_get(w, row, &v1);
        v2.t = view_get(gip->v, gip->gmp[z], &v2);
        if (value_cmp(&v1, &v2, gip->types[2*j], '='))
          break;
      }
      if (j == gip->nc)
        return z;
    }
    o <<= 1;
    if (o > mask)
      o ^= gip->hmpri;
  }
  if (w == gip->v) {
    gip->hmp[i] = gip->gmc->count;
    gip->gmp[gip->gmc->count++] = row;
  }
  return -1;
}

  typedef struct GroupColumn {
    Column c;
    View* v;
    int* emp;
    Colref map;
    Tcl_Obj* mark;
    View** ptrs;
    int cn;
  } GroupColumn;

static Vtype g_group(GroupColumn* gc, int row, Value* out) {
  if (gc->ptrs[row] == 0) {
    int start = gc->emp[row], count = gc->emp[row+1] - start;
    gc->ptrs[row] = view_incRef(make_remapView(gc->v, gc->map, start, count));
  }
///A(out->c >= 0);
  out->u.o = new_viewAt(gc->ptrs[row], out->o, row, gc->cn);
  return Vt_obj;
}
static void group_cleaner(Column* c) {
  free_viewRefs(((GroupColumn*) c)->ptrs, c->count);
  std_cleaner(c);
}

static Column* hash_vals(View* v, const char* types, int rows) {
  Value vb;
  int row, col, nc = (strlen(types)+1)/2;
  Column* outc;
  int *outp = make_intCol(rows, &outc);
  memset(outp, 0, rows * sizeof(int));
  for (col = 0; col < nc; ++col) {
    char type = types[2*col];
    vb.c = col;
    if (type == 'I')
      for (row = 0; row < rows; ++row)
        outp[row] ^= val_toInt(view_get(v, row, &vb), &vb);
    else
      for (row = 0; row < rows; ++row) {
        Vtype t = view_get(v, row, &vb);
        switch (t) {
          case Vt_nil: case Vt_lzy: case Vt_str: break;
          default:      val_toObj(t, &vb); /* fall through */
          case Vt_obj:  vb.u.p = Tcl_GetStringFromObj(vb.u.o, &vb.n);
        }
        if (vb.n > 0) {
          /* similar to Python's stringobject.c */
          const char* p = vb.u.p;
          int i = vb.n, x = *p << 7;
          while (--i >= 0) x = (1000003 * x) ^ *p++;
          outp[row] ^= x ^ vb.n;
        }
      }
  }
  return outc;
}

static int vgroup_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  GroupInfo gi;
  int i, nr, nw, ng, hmsz;
  View *w;
  Column *hwc = 0;
  int fflag = 0, dflag = 0, iflag = 0, lflag = 0;
  Column *smc = 0, *dmc = 0, *fmc = 0, *emc = 0;
  Tcl_Obj *o = Tcl_GetObjResult(sh->cip), *u = 0;
  if (oc < 4 || oc > 5) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "t v w ?g?");
    return TCL_ERROR;
  }
  const char* tags = Tcl_GetString(ov[1]);
  while (*tags)
    switch (*tags++) {
      case 'd': dflag = 1; break;
      case 'f': fflag = 1; break;
      case 'i': iflag = 1; break;
      case 'l': lflag = 1; break;
      case 'x': iflag = -1; break;
      default:  Tcl_SetResult(sh->cip, "bad flag", TCL_STATIC);
                return TCL_ERROR;
    }
  if ((gi.v = as_view(ov[2])) == 0 || (w = as_view(ov[3])) == 0)
    return TCL_ERROR;
  nw = view_size(w);
  gi.types = Tcl_GetString(view_col(0, gi.v->meta, 1));
  nr = view_size(gi.v);
  gi.nc = view_width(gi.v);
  /* prepare hash values */ {
    gi.hvc = hash_vals(gi.v, gi.types, nr);
    gi.hvp = (int*) gi.hvc->ptr;
    if (dflag || iflag)
      hwc = hash_vals(w, gi.types, nw);
  }
  /* prepare empty hash map */ {
    int bits = 0;
    static char adj[] = {
      0, 0, 3, 3, 3, 5, 3, 3, 29, 17, 9, 5, 83, 27, 43, 3,
      45, 9, 39, 39, 9, 5, 3, 33, 27, 9, 71, 39, 9, 5, 83, 0
    };
    for (i = (4*nr)/3; i > 0; i >>= 1)
      ++bits;
    if (bits < 2) bits = 2;
    hmsz = 1 << bits;
    gi.hmpri = hmsz + adj[bits];
    gi.hmp = make_intCol(hmsz, &gi.hmc);
    memset(gi.hmp, 0xff, hmsz * sizeof(int));
  }
  /* set up G and optionally S */ {
    gi.gmp = make_intCol(nr+2, &gi.gmc);
    gi.gmc->count = 0;
    if (fflag) {
      int *smp = make_intCol(nr, &smc);
      for (i = 0; i < nr; ++i) {
        int g = hash_find(gi.v, gi.hvp, i, &gi);
        if (g >= 0) {
          smp[i] = gi.gmp[g];
          gi.gmp[g] = i;
        } else
          smp[i] = -1;
      }
    } else
      for (i = 0; i < nr; ++i)
        hash_find(gi.v, gi.hvp, i, &gi);
    resize_intCol(gi.gmc);
  }
  ng = gi.gmc->count;
  if (dflag) {
    int *dmp = make_intCol(nw, &dmc), *hwp = (int*) hwc->ptr;
    for (i = 0; i < nw; ++i) {
      dmp[i] = hash_find(w, hwp, i, &gi);
      if (dmp[i] < 0 && !lflag)
        dmp[i] = ng;
    }
  }
  if (iflag) {
    int* hwp = (int*) hwc->ptr;
    Buffer buf;
    buf_init(&buf);
    for (i = 0; i < nw; ++i) {
      int g = hash_find(w, hwp, i, &gi);
      if ((iflag < 0 && g < 0) || (iflag > 0 && g >= 0))
        BUF_ADD_INT(buf, i);
    }
    u = buf_asIntCol(&buf, nw);
  }
  col_decRef(gi.hmc);
  col_decRef(gi.hvc);
  if (hwc != 0)
    col_decRef(hwc);
  /* set up the optional E and F maps */
  A(!(fflag && smc == 0));
  if (fflag) {
    int *emp = make_intCol(ng+1+dflag, &emc), *fmp, fe = nr,
        *gmp = (int*) gi.gmc->ptr, *smp = (int*) smc->ptr;
    emp[ng] = nr;
    /* possible optimization: keep only referenced groups if dflag is set */
    if (dflag)
      emp[ng+1] = nr;
    fmp = make_intCol(nr, &fmc);
    for (i = ng; --i >= 0; ) {
      int j = gmp[i];
      do { fmp[--fe] = j; j = smp[j]; } while (j >= 0);
      emp[i] = fe;
    }
    A(fe == 0);
    col_decRef(smc);
  }
  l_append(o, new_col(gi.gmc, -1));
  if (fflag) {
    l_append(o, new_col(emc, -1));
    l_append(o, new_col(fmc, -1));
  }
  if (dflag)
    l_append(o, new_col(dmc, -1));
  /* create an optional group column */
  if (fflag && oc > 4) {
    GroupColumn* gc;
    int bytes = (ng+1) * sizeof(View*);
    Tcl_Obj* mo = new_list4(ov[4], l_index(o, 1), l_index(o, 2), l_index(o, 3));
    gc = MAKE_COL(GroupColumn, mo, 0, ng, g_group);
    gc->v = as_view(ov[4]);
    gc->emp = (int*) emc->ptr;
    gc->map = as_col(l_index(o, 2));
    gc->mark = sh->none;
    gc->cn = gi.nc;
    gc->c.cleaner = group_cleaner;
    gc->ptrs = (View**) memset(ckalloc(bytes), 0, bytes);
    l_append(o, new_col(&gc->c, -1));
  }
  if (iflag)
    l_append(o, u);
  return TCL_OK;
}

  typedef struct FlatColumn {
    Column c;
    Tcl_Obj** av;
    int bflag, ac, *offp;
  } FlatColumn;

static Vtype g_flat(FlatColumn* vfv, int row, Value* vp) {
  /* TODO: simplify, can use a column-wise view in some cases */
  View* w;
  int e, i, orow = row;
  if (vfv->bflag < 0)
    row = vp->c;
  e = row;
  i = vfv->offp[row];
  if (i < 0) {
    int j = i;
    i = vfv->offp[row+i];
    row = -j;
  } else
    row = 0;
  if (vfv->bflag > 1 && vfv->offp[e+1] >= 0) {
    row = i;
    i = vfv->ac - 1;
  }
  if (vfv->bflag < 0) {
    vp->c = row;
    row = orow;
  }
  w = as_view(vfv->av[i]); A(w != 0);
  return view_get(w, row, vp);
}
static Tcl_Obj* make_flatView(int bflag, Tcl_Obj* views, Tcl_Obj* meta) {
  Buffer buf;
  FlatColumn* vfv;
  Tcl_Obj *keep, **av, *o, *map;
  int i, j, n, n0 = 0, ac, step = bflag > 1;
  Shared* sh = get_shared();
  if (Tcl_ListObjGetElements(sh->cip, views, &ac, &av) != TCL_OK)
    return 0;
  buf_init(&buf);
  for (i = 0; i < ac; ++i) {
    View* v = as_view(av[i]);
    if (v == 0)
      return 0;
    if (bflag < 0) {
      if (i == 0)
        n0 = view_size(v);
      else if (view_size(v) < n0) {
        Tcl_SetResult(sh->cip, "not enough rows", TCL_STATIC);
        return 0;
      }
      if (meta == 0)
        meta = Tcl_NewListObj(0, 0);
      l_append(meta, new_view(v->meta, 0));
      v = v->meta; A(v != 0);
    } else if (meta == 0)
      meta = new_view(v->meta, 0);
    n = view_size(v) + step;
    if (n > 0) {
      BUF_ADD_INT(buf, i);
      for (j = 1; j < n; ++j)
        BUF_ADD_INT(buf, -j);
    }
  }
  n = BUF_BYTES(buf) / sizeof(int) - step*ac;
  A(meta != 0);
  if (bflag < 0) {
    View* v;
    int k = 0;
    meta = make_flatView(0, meta, 0);
    v = make_view(as_view(meta), n0);
    for (i = 0; i < ac; ++i) {
      View *w = as_view(av[i]);
      int n = view_width(w);
      for (j = 0; j < n; ++j)
        view_setCol(v, k++, w->cols[j]);
    }
    A(k == view_width(v));
    return new_view(v, 0);
  }
  map = buf_asIntCol(&buf, 0);
  keep = new_list4(map, views, 0, 0);
  vfv = MAKE_COL(FlatColumn, keep, 0, n, g_flat);
  vfv->bflag = bflag;
  vfv->ac = ac;
  vfv->av = av;
  vfv->offp = (int*) as_col(map).c->ptr;
  o = new_view(make_indirView(as_view(meta), &vfv->c), 0);
  return bflag != 1 ? o : new_list4(o, map, 0, 0);
}
static int vflat_vop(Shared* sh, int oc, Tcl_Obj* const* ov) {
  Tcl_Obj* o;
  int bflag = 0;
  if (oc < 3 || oc > 4) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "l b ?m?");
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(sh->cip, ov[2], &bflag) != TCL_OK)
    return TCL_ERROR;
  o = make_flatView(bflag, ov[1], oc > 3 ? ov[3] : 0);
  if (o == 0)
    return TCL_ERROR;
  Tcl_SetObjResult(sh->cip, o);
  return TCL_OK;
}

static void l_appendTag(Tcl_Obj* o, const char* tag, Tcl_Obj* v) {
  l_append(o, Tcl_NewStringObj(tag, -1));
  l_append(o, v);
}
static void view_funTags(Tcl_Obj* r, View* v) {
  struct { const char* n; Sizer f; } sizers[] = {
    {"std_sizer",std_sizer},
    {"cmd_sizer",cmd_sizer},
    {"ref_sizer",(Sizer)ref_sizer},
    {0,0}
  };
  int i;
  for (i = 0; sizers[i].n != 0; ++i)
    if (v->sizer == sizers[i].f)
      l_appendTag(r, "sizer", Tcl_NewStringObj(sizers[i].n, -1));
}
static void col_funTag(Tcl_Obj* r, Column* c) {
  struct { const char* n; Getter f; } getters[] = {
    {"g_sub",(Getter)g_sub},
    {"g_var",(Getter)g_var},
    {"g_i0",g_i0},
    {"g_i1",g_i1},
    {"g_i2",g_i2},
    {"g_i4",g_i4},
    {"g_i8",g_i8},
    {"g_i16",g_i16},
    {"g_i16r",g_i16r},
    {"g_i32",g_i32},
    {"g_i32r",g_i32r},
    {"g_i64",g_i64},
    {"g_i64r",g_i64r},
    {"g_f32",g_f32},
    {"g_f32r",g_f32r},
    {"g_f64",g_f64},
    {"g_f64r",g_f64r},
    {"g_obj",g_obj},
    {"g_svec",g_svec},
    {"g_iota",g_iota},
    {"g_view",(Getter)g_view},
    {"g_cmd",g_cmd},
    {"g_remap",(Getter)g_remap},
    {"g_step",(Getter)g_step},
    {"g_ref",(Getter)g_ref},
    {"g_group",(Getter)g_group},
    {"g_flat",(Getter)g_flat},
    {0,0}
  };
  struct { const char* n; Cleaner f; } cleaners[] = {
    {"std_cleaner",std_cleaner},
    {"view_cleaner",view_cleaner},
    {"sub_cleaner",sub_cleaner},
    {"ref_cleaner",ref_cleaner},
    {"group_cleaner",group_cleaner},
    {0,0}
  };
  int i;
  for (i = 0; getters[i].n != 0; ++i)
    if (c->getter == getters[i].f) {
      l_appendTag(r, "getter", Tcl_NewStringObj(getters[i].n, -1));
      break;
    }
  for (i = 0; cleaners[i].n != 0; ++i)
    if (c->cleaner == cleaners[i].f)
      l_appendTag(r, "cleaner", Tcl_NewStringObj(cleaners[i].n, -1));
}
static int vinfo_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  Tcl_Obj *obj, *r = Tcl_NewObj();
  const char* fmt;

  if (oc < 2 || oc > 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "?fmt? obj");
    return TCL_ERROR;
  }

  fmt = oc > 2 ? Tcl_GetString(*++ov) : "";
  obj = ov[1];

  Tcl_SetObjResult(sh->cip, r);
  l_appendTag(r, "bytes", new_int(obj->bytes != 0 ? obj->length : -1));
  l_appendTag(r, "type", obj->typePtr != 0
                                  ? Tcl_NewStringObj(obj->typePtr->name, -1)
                                  : sh->none);
  l_appendTag(r, "refs", new_int(obj->refCount));

  if (*fmt == '*')
    fmt = obj->typePtr == &view_type ? "ef" :
          obj->typePtr == &col_type ? "cat" :
          obj->typePtr == &lazy_type ? "pr" : "";

  while (*fmt) {
    char f = *fmt++;
    switch (f) {
      case 'e': case 'f': case 'o': case 'd': {
        View *v = as_view(obj);
        if (v == 0)
          return TCL_ERROR;
        switch (f) {
          case 'e': l_appendTag(r, "vsize", new_int(v->size)); break;
          case 'f': view_funTags(r, v); break;
          case 'o': l_appendTag(r, "orig", VIEW_ORG(obj)->def); break;
          case 'd': l_appendTag(r, "deps", VIEW_ORG(obj)->deps); break;
        }
        break;
      }
      case 'c': case 'a': case 't': {
        Column *c = as_col(obj).c;
        switch (f) {
          case 'c': l_appendTag(r, "count", new_int(c->count)); break;
          case 'a': l_appendTag(r, "aux", c->aux); break;
          case 't': col_funTag(r, c); break;
        }
        break;
      }
      case 'p': if (obj->typePtr == &lazy_type)
                  l_appendTag(r, "ptr", Tcl_NewStringObj(LAZY_PTR(obj), -1));
                break;
      case 'r': if (obj->typePtr == &lazy_type)
                  l_appendTag(r, "obj", LAZY_OBJ(obj));
                break;
      default:  Tcl_ResetResult(sh->cip);
                Tcl_AppendResult(sh->cip, "error in format: ", --fmt, 0);
                return TCL_ERROR;
    }
  }
  return TCL_OK;
}

/* - - -  TRIALS  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int sum_8_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int e, i, col, s = 0;
  View* v;
  Column* c;
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view colnum");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  e = Tcl_GetIntFromObj(0, ov[2], &col); A(e == TCL_OK);
  c = v->cols[col].c;
  if (c->getter != g_i8) {
    Tcl_SetResult(sh->cip, "column is not of type 'i8'", TCL_STATIC);
    return TCL_ERROR;
  }
  for (i = 0; i < c->count; ++i)
    s += c->ptr[i];
  Tcl_SetObjResult(sh->cip, Tcl_NewIntObj(s));
  return TCL_OK;
}
static int sum_8w_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int e, i, col;
  View* v;
  Column* c;
  Tcl_WideInt s = 0;
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view colnum");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  e = Tcl_GetIntFromObj(0, ov[2], &col); A(e == TCL_OK);
  c = v->cols[col].c;
  if (c->getter != g_i8) {
    Tcl_SetResult(sh->cip, "column is not of type 'i8'", TCL_STATIC);
    return TCL_ERROR;
  }
  for (i = 0; i < c->count; ++i)
    s += c->ptr[i];
  Tcl_SetObjResult(sh->cip, Tcl_NewWideIntObj(s));
  return TCL_OK;
}
static int sum_v_cmd(Shared* sh, int oc, Tcl_Obj* const* ov) {
  int e, i, n;
  View* v;
  Value vb;
  Tcl_WideInt s = 0;
  if (oc != 3) {
    Tcl_WrongNumArgs(sh->cip, 1, ov, "view colnum");
    return TCL_ERROR;
  }
  if ((v = as_view(ov[1])) == 0)
    return TCL_ERROR;
  e = Tcl_GetIntFromObj(0, ov[2], &vb.c); A(e == TCL_OK);
  n = view_size(v);
  for (i = 0; i < n; ++i)
    s += val_toInt(view_get(v, i, &vb), &vb);
  Tcl_SetObjResult(sh->cip, Tcl_NewWideIntObj(s));
  return TCL_OK;
}

/* - - -  SETUP  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void done_shared(ClientData cd) {
  Shared* sh = (Shared*) cd;
  A(sh->stash != 0);
  Tcl_DecrRefCount(sh->stash);
  sh->stash = 0;
}
static Tcl_Obj* init_const(const char* name) {
  return l_append(get_shared()->stash, Tcl_NewStringObj(name, -1));
}
static void init_shared() {
  int i;
  Tcl_Obj* o = Tcl_NewObj();
  Shared* sh = get_shared();
  A(sh->stash == 0);
  sh->stash   = incr_ref(Tcl_NewObj());
  sh->none    = init_const("");
  sh->hash    = init_const("#");
  sh->stype   = init_const("S");
  sh->vtype   = init_const("V");
  sh->view    = init_const("view");
  sh->vops    = init_const("vops");
  sh->mdef    = init_const("mdef");
  sh->vdata   = init_const("vdata");
  sh->remap   = init_const("remap");
  sh->at      = init_const("at");
  sh->meta    = init_const("meta");
  sh->mapf    = init_const("mapf");
  sh->mmeta   = 0;
  /* small integers are pre-allocated as objects to improve object sharing */
  l_append(sh->stash, o);
  sh->tinies = 128 + (Tcl_Obj**) Tcl_SetByteArrayLength(o,256*sizeof(Tcl_Obj*));
  for (i = -128; i <= 127; ++i)
    sh->tinies[i] = l_append(sh->stash, Tcl_NewIntObj(i));
  { /* tricky fixup of the meta meta view definition */
    View *z, *m = sh->mmeta = make_view(0, 3);
    view_setCol(m, 0, as_col(Tcl_NewStringObj("name type subv", -1)));
    view_setCol(m, 1, as_col(Tcl_NewStringObj("S S V", -1)));
    view_setCol(m, 2, as_col(Tcl_NewStringObj("{} {} {}", -1)));
    m->meta = view_incRef(m); /* close the circle */
    z = sh->zmeta = view_incRef(make_view(sh->mmeta, 0));
    for (i = 0; i < 3; ++i)
      view_setCol(z, i, m->cols[i]);
  }
  unget_shared(done_shared, (ClientData) sh);
}

typedef int (*CmdProc)(Shared*,int,Tcl_Obj*const*);

static int wrap_cmd(ClientData cd, Tcl_Interp *ip, int oc, Tcl_Obj* const* ov) {
  int e;
  Shared* sh = get_shared();
  Tcl_Interp* oip = sh->cip;
  sh->cip = ip;
  e = ((CmdProc) cd)(sh, oc, ov);
  sh->cip = oip;
  return e;
}
static void def_cmd(Tcl_Interp* ip, const char* name, CmdProc fun) {
  Tcl_CreateObjCommand(ip, name, wrap_cmd, (ClientData) fun, 0);
}

extern int Vlerq_Init(Tcl_Interp *ip) {
#ifndef NDEBUG
  Value v;
  A(sizeof v.u.i == 4);
  A(sizeof v.u.f == 4);
  A(sizeof v.u.q == 8);
  A(sizeof v.u.d == 8);
  A(sizeof v.u == 8);
#endif
  if (MyInitStubs(ip) == 0 || Tcl_PkgRequire(ip,"Tcl","8.4",0) == 0)
    return TCL_ERROR;
  def_cmd(ip, "::view", view_cmd);
  def_cmd(ip, "::vlerq::colnum", colnum_vop);
  def_cmd(ip, "::vget", vget_vop);
  def_cmd(ip, "::vlerq::loop", loop_vop);
  def_cmd(ip, "::vlerq::mdef", mdef_vop);
  def_cmd(ip, "::vlerq::vdata", vdata_vop);
  def_cmd(ip, "::vlerq::mapf", mapf_vop);
  def_cmd(ip, "::vlerq::load", mapf_vop);
  def_cmd(ip, "::vlerq::remap", remap_vop);
  def_cmd(ip, "::vlerq::cremap", cremap_vop);
  def_cmd(ip, "::vlerq::vstep", vstep_vop);
  def_cmd(ip, "::vlerq::ref", ref_vop);
  def_cmd(ip, "::vlerq::at", at_vop);
  def_cmd(ip, "::vlerq::sortmap", sortmap_vop);
  def_cmd(ip, "::vlerq::find", find_vop);
  def_cmd(ip, "::vlerq::ibytes", ibytes_cmd);
  def_cmd(ip, "::vlerq::vgroup", vgroup_cmd);
  def_cmd(ip, "::vlerq::vflat", vflat_vop);
  def_cmd(ip, "::vlerq::vinfo", vinfo_cmd);
  def_cmd(ip, "::vlerq::sum_8", sum_8_cmd);
  def_cmd(ip, "::vlerq::sum_8w", sum_8w_cmd);
  def_cmd(ip, "::vlerq::sum_v", sum_v_cmd);
  Tcl_Eval(ip, "array set ::vops { "
    "colnum ::vlerq::colnum "
    "get ::vget "
    "loop ::vlerq::loop "
    "mdef {::vlerq::mdef +} "
    "vdata {::vlerq::vdata +} " 
    "mapf ::vlerq::mapf "
    "load {::vlerq::load +} "
    "remap {::vlerq::remap +} "
    "cremap {::vlerq::cremap +} "
    "vstep {::vlerq::vstep +} "
    "ref {::vlerq::ref +} "
    "at {::vlerq::at +} "
    "sortmap ::vlerq::sortmap "
    "find ::vlerq::find "
    "vflat ::vlerq::vflat "
  "}");
  if (get_shared()->stash == 0)
    init_shared();
  return Tcl_PkgProvide(ip, "vlerq", "3");
}
