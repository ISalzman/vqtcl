/* Tcl interface to Thrive -- jcw 2004-11-19 */

#define USE_TCL_STUBS 1
#include <tcl.h>

#define NAME "Tcl"

/* %includeDir<.>% */
/* %includeDir<../noarch>% */
/* %include<thrive.h>% */
#include "thrive.h"
/* %include<>% */

#ifdef BUILD_thrive
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

/* stub interface code, removes the need to link with libtclstub*.a */
#ifdef USE_TCL_STUBS
/* %include<stubtcl.h>% */
#include "stubtcl.h"
/* %include<>% */
#else
#define MyInitStubs(x) 1
#endif

static char version[] = {
  VERSION/100+'0','.',(VERSION/10)%10+'0',VERSION%10+'0',0
};

/* no need to be mutex-protected, as long as per-interp seq is unique */
static int seqno = 0;

static void
th_FreeIntRep(Tcl_Obj *o) {
  P w = (P) o->internalRep.twoPtrValue.ptr1;
  I i = (I) o->internalRep.twoPtrValue.ptr2;
  dropRef(w,i);
}

static void
th_DupIntRep(Tcl_Obj *o, Tcl_Obj *p) {
  /* ? */
}

static void
th_UpdateString(Tcl_Obj *o) {
  char buf[25];
  sprintf(buf,"^%ld",(I)o->internalRep.twoPtrValue.ptr2);
  o->length = strlen(buf);
  o->bytes = ckalloc(o->length + 1);
  strcpy(o->bytes,buf);
}

static int
th_SetFromAny(Tcl_Interp *ip, Tcl_Obj *o) {
  /* ? */
  return TCL_OK;
}

static Tcl_ObjType th_type = {
  "thriveRef", th_FreeIntRep, th_DupIntRep, th_UpdateString, th_SetFromAny
};

static Tcl_Obj*
th_NewRefObj(P p, I i) {
  P w = Work(p);
  I r = makeRef(p,i);
  Tcl_Obj *o = Tcl_NewObj();
  o->bytes = 0;
  o->typePtr = &th_type;
  o->internalRep.twoPtrValue.ptr1 = w;
  o->internalRep.twoPtrValue.ptr2 = (void*) r;
  return o;
}

static Tcl_Obj*
th_Get(Tcl_Interp *ip, P p, I i) {
  P w = Work(p);
  Tcl_Obj *g;
  if (i == -1) i += Length(p);
  switch (Type(p[i])) {
    case Tn:
      /* nil with non-zero .i is for special markers which throw an error */
      if (p[i].i) {
	Tcl_SetObjResult(ip,Tcl_NewLongObj(p[i].i));
	return 0;
      }
      return th_Get(ip,Wn.p,Dat);
    case Ti:
      return Tcl_NewLongObj(p[i].i);
    case Ts:
      return Tcl_NewByteArrayObj((US)strAt(p[i]),lenAt(p[i]));
    case Td: {
      P q = p[i].p;
      Tcl_Obj *o = Tcl_NewListObj(0,0);
      I j, n = Length(q);
      F f = (F) Getter(q);
      /* call getter on each element, but don't allow it to call back */
      for (j = 0; j < n; ++j) {
	/* TODO this test is flawed, it does not catch compound words
	   (should revisit the whole return/fetch/eval logic one day) */
	if (f(q,j)) {
	  Z printf("Get? p 0x%lx i %ld q 0x%lx j %ld\n",(U)p,i,(U)q,j);
	  Tcl_DecrRefCount(o);
	  Tcl_SetResult(ip,"cannot handle callback",TCL_STATIC);
	  return 0; 
	}
	g = th_Get(ip,w,-1);
	if (g == 0) { Tcl_DecrRefCount(o); return 0; }
	Tcl_ListObjAppendElement(ip,o,g);
	nilPush(w,-1); /* pop */
      }
      return o;
    }
    case Ta: {
      P q = p[i].p;
      Tcl_Obj *o = Tcl_NewListObj(0,0);
      int j, n = Length(q);
      if (n > Limit(q)) n = Limit(q);
      for (j = 0; j < n; ++j) {
	g = th_Get(ip,q,j);
	if (g == 0) { Tcl_DecrRefCount(o); return 0; }
	Tcl_ListObjAppendElement(ip,o,g);
      }
      return o;
    }
    case To:
      return th_NewRefObj(p,i);
  }
  g = th_Get(ip,Wn.p,Tag);
  if (g != 0) Tcl_SetObjResult(ip,g);
  return 0;
}

static I th_getref(Tcl_Interp *ip, P w, Tcl_Obj *o) {
  if (o->typePtr != &th_type) {
    Tcl_SetResult(ip,"not a reference",TCL_STATIC);
    return 0;
  }
  if (o->internalRep.twoPtrValue.ptr1 != w) {
    Tcl_SetResult(ip,"ref error, wrong workspace",TCL_STATIC);
    return 0;
  }
  return (I)o->internalRep.twoPtrValue.ptr2;
}
static const char*
th_Push(Tcl_Interp *ip, P p, const char* s, int oc, Tcl_Obj *const ov[]) {
  int i, n;
  I t;
  const char *q = s, *r;
  for (i = 0; i < oc; ++i) {
    Tcl_Obj *o = ov[i];
    if (*q == 0 || *q == ')') q = s;
    if (o->length == 0 && o->bytes != 0 && 'a' <= *q && *q <= 'z') {
      nilPush(p,1);
      ++q;
      continue;
    }
    switch (*q++) {
      case 'a':
	if (Tcl_GetLongFromObj(0,o,&t) == TCL_OK) {
	  intPush(p,t);
	  break;
	} /* else fall through */
      case 's':
      case 'S':
	r = Tcl_GetStringFromObj(o,&n);
        strPush(p,r,n);
	break;
      case 'b':
      case 'B':
	r = (S)Tcl_GetByteArrayFromObj(o,&n);
        strPush(p,r,n);
	break;
      case 'i':
      case 'I':
	if (Tcl_GetLongFromObj(ip,o,&t) == TCL_ERROR)
	  return 0;
	intPush(p,t);
	break;
      case 'v':
      case 'V':
	t = th_getref(ip,Work(p),o);
	if (t == 0) return 0;
	pushRef(p,t);
	break;
      case 'n':
	nilPush(p,1);
	break;
      case '(': {
	int oc2;
	Tcl_Obj **ov2;
	if (Tcl_ListObjGetElements(ip,o,&oc2,&ov2) == TCL_ERROR)
	  return 0;
	q = th_Push(ip,boxPush(p,oc2),q,oc2,ov2);
	if (q == 0)
	  return 0;
	while (*q && *q++ != ')')
	  ;
	break;
      }
      default:
	Tcl_SetResult(ip,"bad format spec",TCL_STATIC);
	return 0;
    }
  }
  return q;
}

static int
th_ObjCmd(ClientData cd, Tcl_Interp *ip, int oc, Tcl_Obj *const ov[]) {
  static const char *cmds[] = {
    "pop","run","eval","find","push","load","type",0
  };
  int id;
  Tcl_Obj *g;
  I n = 0;
  S s = 0;
  P w = (P) cd;
  if (oc < 2) {
    Tcl_WrongNumArgs(ip,1,ov,"cmd ...");
    return TCL_ERROR;
  }
  if (Tcl_GetIndexFromObj(ip,ov[1],cmds,"option",0,&id) == TCL_ERROR)
    return TCL_ERROR;
  if (id > 1 && oc < 3) {
    Tcl_WrongNumArgs(ip,2,ov,"arg ...");
    return TCL_ERROR;
  }
  switch (id) {
    case 1: /* run ?func? */
      if (oc >= 3) {
	n = th_getref(ip,w,ov[2]);
	if (n == 0) return TCL_ERROR;
	pushRef(w,n);
	n = 1;
      }
      break;
    case 2: /* eval token */
    case 3: /* find token */
      strPush(w,Tcl_GetStringFromObj(ov[2],0),~0);
      n = findSym(w);
      break;
  }
  switch (id) {
    case 0: /* pop */
      g = th_Get(ip,w,-1);
      nilPush(w,-1);
      if (g == 0) return TCL_ERROR;
      Tcl_SetObjResult(ip,g);
      return TCL_OK;
    case 1: /* run ?func? */
      n = runWork(w,n);
      break;
    case 2: /* eval token */
      if (n >= 0) evalSym(w,n);
      break;
    case 3: /* find token */
      /*TODO ugly code, breaks encapsulation of Wv */
      if (n >= Wv.i) {
	Tcl_SetResult(ip,"symbol not found",TCL_STATIC);
	return TCL_ERROR;
      }
      Tcl_SetObjResult(ip,th_NewRefObj(Wv.p,n));
      return TCL_OK;
    case 4: /* push fmt ?...? */
      s = Tcl_GetStringFromObj(ov[2],0);
      return th_Push(ip,w,s,oc-3,ov+3) != 0 ? TCL_OK : TCL_ERROR;
    case 5: { /* load script */
      int i;
      M q = Tcl_GetStringFromObj(ov[2],&i);
      M r = q = strcpy(ckalloc(i+1),q);
      if (*q != '\\' && *q != '#')
	for (;;) {
	  s = scanSym(&r);
	  if (s == 0) break;
	  if (*s != '#') {
	    strPush(w,s,~0);
	    n = findSym(w);
	    if (n >= 0) evalSym(w,n);
	    if (runWork(w,0)>>1) {
	      Tcl_SetResult(ip,(M)s,TCL_VOLATILE);
	      break;
	    }
	  }
	}
      ckfree(q);
      return s == 0 ? TCL_OK : TCL_ERROR;
    }
    case 6: { /* type ref */
      n = th_getref(ip,w,ov[2]);
      if (n == 0) return TCL_ERROR;
      /*TODO ugly code, breaks encapsulation of Wx */
      n = Type(Wx.p[n]);
      break;
    }
  }
  Tcl_SetObjResult(ip,Tcl_NewLongObj(n));
  return TCL_OK;
}

static void
th_DelProc(ClientData cd) {
  endWork((P) cd);
}

static int
new_Cmd(ClientData cd, Tcl_Interp *ip, int oc, Tcl_Obj *const ov[]) {
  if (oc != 1) Tcl_WrongNumArgs(ip,1,ov,""); else {
    char buf[40];
    P w = newWork();
    sprintf(buf,"::thrive_%d",++seqno);
    Tcl_CreateObjCommand(ip,buf,th_ObjCmd,(ClientData)w,th_DelProc);
    Tcl_SetResult(ip,buf,TCL_VOLATILE);
    if (Tcl_GetObjType("thriveRef") == 0)
      Tcl_RegisterObjType(&th_type);
    return TCL_OK;
  }
  return TCL_ERROR;
}

#ifndef CRITCL
extern DLLEXPORT int
Thrive_Init(Tcl_Interp *ip) {
  if (MyInitStubs(ip) == 0 || Tcl_PkgRequire(ip,"Tcl","8.1",0) == 0)
    return TCL_ERROR;
  Tcl_CreateObjCommand(ip,"::thrive",new_Cmd,0,0);
  return Tcl_PkgProvide(ip,"thrive",version);
}
#endif

/* Read a 4-byte big-endian int from current file pos */
L I read4be(Tcl_Channel fd) {
  UC buf[4];
  return Tcl_Read(fd, (M)buf, 4) != 4 ? 0 :
		    (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

/* The Ratcl entry point implements "Starkit-like" functionality, loading
 * and running a Tcl script, setting up a Thrive VM, and then loading the
 * rest of the necessary Thrill code to properly set up the Ratcl system.
 */

extern DLLEXPORT int
Ratcl_Init(Tcl_Interp *ip) {
  I ok = 0;
  if (Thrive_Init(ip) != TCL_ERROR) {
    if (Tcl_Eval(ip,"foreach _ [info loaded] {"
		      "switch -- [lindex $_ 1] Ratcl { return [lindex $_ 0] }"
		    "}") != TCL_RETURN)
      ok = 1;
    else {
      Tcl_Channel fd = Tcl_OpenFileChannel(ip,Tcl_GetStringResult(ip),"r",0);
      if (fd != 0) {
	I ulen, zlen, mark = 0x74586953; /* tXiS */
	if (Tcl_SetChannelOption(ip,fd,"-translation","binary") == TCL_OK &&
	    Tcl_Seek(fd,-12,SEEK_END) >= 0 &&
	    ((ulen = read4be(fd)) == mark ||
	     (Tcl_Seek(fd,-ulen-20,SEEK_CUR) >= 0 && read4be(fd) == mark))) {
	  ulen = read4be(fd);
	  zlen = read4be(fd);
	  if (Tcl_Seek(fd,-zlen-12,SEEK_CUR) >= 0) {
	    P w = newWork();
	    P p = extCode(w,"xdez",dezExt);
	    M ubuf = strPush(w,0,ulen);
	    if (ulen == zlen) 
	      ok = Tcl_Read(fd,ubuf,ulen) == ulen;
	    else {
	      M zbuf = strPush(w,0,zlen);
	      ok = Tcl_Read(fd,zbuf,zlen) == zlen && dezExt(p,0) == 0;
	    }
	    if (ok) 
	      ok = Tcl_EvalEx(ip,ubuf,ulen,TCL_EVAL_DIRECT) != TCL_ERROR;
	    endWork(w);
	  }
	}
	ok = Tcl_Close(ip,fd) == TCL_OK;
      }
    }
  }
  return ok ? TCL_OK : TCL_ERROR;
}
