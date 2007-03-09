/* thrive.h -=- Thrive virtual machine */

#if 0
/* %ext-on% <-- directives like these are processed by exthdr.tcl */
/* t4ext.h -- thrive public extension api, auto-extracted from thrive.h */
/* %ext-off% */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NAME
#define NAME "Thrive"
#endif

#define VERSION 223

#define MAXCODE  8000   /* not a hard limit, as "{...}" code is not counted */
#define MAXSYMS   300   /* will be grown as needed, see the test in findSym */
#define MAXGETF   100   /* with room for large number of dynamic extensions */
#define MAXREGS    30   /* that's 25 "W<letter>" registers and a few spares */
#define MAXVARS    70   /* globals variables, of which there should be few! */
#define MAXWORK   500   /* hard limit for data and return stacks: 500-70-30 */

#ifdef TDEBUG
#include <assert.h>
#define A(x) assert(x)
#define D
#else
#define A(x)
#define D Z
#endif

#define L static
#define Z if (0) 
/* %ext-on% */
#define V void

typedef char C; typedef C* M; typedef const C* S;
typedef unsigned char UC; typedef UC* UM; typedef const UC* US;
typedef short H; typedef unsigned short UH;
typedef int Q; typedef unsigned int UQ;
#if __INT_MAX__ < __LONG_MAX__
  typedef long W;
#elif defined(__LONG_LONG_MAX__)
  typedef long long W;
#elif defined(WIN32)
  typedef __int64 W;
#else
#include <limits.h>
#if INT_MAX < LONG_MAX
  typedef long W;
#elif defined(LLONG_MAX)
  typedef long long W;
#endif
#endif
typedef long I; typedef unsigned long U;
typedef float R; typedef double E;
typedef struct B { I i; struct B* p; } B, *P;
typedef I (*F)(P,I);

typedef struct J {
  I f_VERSION;
  P (*f_newWork)(void);
  I (*f_runWork)(P,I);
  V (*f_endWork)(P);
  I (*f_intPush)(P,I);
  M (*f_strPush)(P,S,I);
  P (*f_boxPush)(P,I);
  V (*f_nilPush)(P,I);
  I (*f_makeRef)(P,I);
  V (*f_pushRef)(P,I);
  V (*f_dropRef)(P,I);
  S (*f_cString)(P,I);
  S (*f_scanSym)(M*);
  I (*f_findSym)(P);
  V (*f_evalSym)(P,I);
  P (*f_extCode)(P,S,F);
} J;

#define HEAD 8
enum {
  Get = -HEAD, Cnt, Str, Fin, Tag, Adr, Dat, Len
};
enum {
  Tn, Ti, Ts, Td, Ta, To, Te
};

#define Chain(x)  ((x)[Get].p)
#define Getter(x) ((x)[Get].i)
#define Work(x)   ((x)[Cnt].p)
#define Alloc(x)  ((x)[Cnt].i)
#define Limit(x)  (Alloc(x)/(I)sizeof(B))
#define Base(x)   ((x)[Str].p)
#define Offset(x) ((x)[Str].i)
#define String(x) ((S)Base(x)+Offset(x)) /* evals x twice */
#define Length(x) ((x)[Len].i)

#define strAt(x) String((x).p) /* evals x twice */
#define lenAt(x) Length((x).p)
#define isInt(x) (Alloc((x).p)==-1)
#define isVec(x) (Offset((x).p)==-1)
#define isStr(x) (!isInt(x)&&!isVec(x)) /* evals x twice */

#define Type(x) \
  (isInt(x) ? Ti : \
    ((x).i != -1 || Getter((x).p) == Getter(Wz.p) \
      ? (isVec(x) ? To : strAt(x) != 0 ? Ts : Tn) \
      : (isVec(x) ? Ta : strAt(x) != 0 ? Td : Te)))
/* %ext-off% */
#if 0
/* %ext-on% */

#define VERSION	jVec->f_VERSION
#define newWork	jVec->f_newWork
#define runWork	jVec->f_runWork
#define endWork	jVec->f_endWork
#define intPush	jVec->f_intPush
#define strPush	jVec->f_strPush
#define boxPush	jVec->f_boxPush
#define nilPush	jVec->f_nilPush
#define makeRef	jVec->f_makeRef
#define pushRef	jVec->f_pushRef
#define dropRef	jVec->f_dropRef
#define cString	jVec->f_cString
#define scanSym	jVec->f_scanSym
#define findSym	jVec->f_findSym
#define evalSym	jVec->f_evalSym
#define extCode	jVec->f_extCode

#define DEFINE(n) J* jVec; I n
#define extIni(x) jVec = (J*) strAt(x[16]/*Wj*/)

extern J* jVec;
/* t4ext.h -- end */
/* %ext-off% */
#endif

#define Push(x,y) ((x)[Length(x)++]=(y))

#define whsp(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

#define BPI	((I)sizeof(I)*8)

#define WREGS "dprtnzlsvicgwuhefjxkqoabmy"

#define Wd w[Len]	/* data stack */
/*%renumber<#def>%*/
#define Wp w[0]		/* program counter */
#define Wr w[1]		/* return stack */
#define Wt w[2]		/* token state */
#define Wn w[3]		/* nil */
#define Wz w[4]		/* zero integer */
#define Wl w[5]		/* last definition */
#define Ws w[6]		/* 1/3 symbols */
#define Wv w[7]		/* 2/3 values */
#define Wi w[8]		/* 3/3 immediate */
#define Wc w[9]		/* code vector */
#define Wg w[10]	/* getters */
#define Ww w[11]	/* writable variables */
#define Wu w[12]	/* gc used bytes */
#define Wh w[13]	/* gc high limit */
#define We w[14]	/* escape handler */
#define Wf w[15]	/* fault handler */
#define Wj w[16]	/* jump table for stubs */
#define Wx w[17]	/* external reference */
#define Wk w[18]	/* access counters */
#define Wq w[19]	/* quiet execution */
#define Wo w[20]	/* object or offset */
#define Wa w[21]	/* spare: a register */
#define Wb w[22]	/* spare: b register */
#define Wm w[23]	/* spare: weak map */
#define Wy w[24]	/* spare: ? */
#define WMAX 25
/*%renumber<>%*/

#define GREGS "ibrpevjctu"

/*%renumber<#def>%*/
#define Gi Wg.p[0]	/* intGet */
#define Gb Wg.p[1]	/* boxGet */
#define Gr Wg.p[2]	/* rawGet */
#define Gp Wg.p[3]	/* priGet */
#define Ge Wg.p[4]	/* exeGet */
#define Gv Wg.p[5]	/* vofGet */
#define Gj Wg.p[6]	/* jmpGet */
#define Gc Wg.p[7]	/* cndGet */
#define Gt Wg.p[8]	/* tstGet */
#define Gu Wg.p[9]	/* usrGet */
#define GMAX 10
/*%renumber<>%*/

#define Wsubd w[Wd.i-2]
#define Wtopd w[Wd.i-1]
#define Wpopd w[--Wd.i]
#define Wpushd w[Wd.i++]

L V extInit(P); /* forward */

L I intGet(P p, I c) {
  P w = Work(p);
  Wpushd.p = p; Wtopd.i = c;
  return 0;
}
L I boxGet(P p, I c) {
  P w = Work(p);
  if (c < 0 || c >= Limit(p)) {
    Wpushd.p = p; Wtopd.i = c;
  } else
    Wpushd = p[c];
  return 0;
}
L V gcMark(P p) {
  I n = Offset(p) == -1 ? Limit(p) : 0;
  Work(p) = 0;
  while (--n >= Str) if (Work(p[n].p) != 0) gcMark(p[n].p);
}
L V gcSweep(P w, I f) {
  P p = w;
  Work(p) = 0;
  for (;;) {
    P q = Chain(p);
    if (Work(q) != 0) {
      I n = Base(q) == q ? Alloc(q) : 0;
      Chain(p) = Chain(q);
      Wu.i -= HEAD * sizeof(B) + n;
      Z printf("     free 0x%08lx len %ld\n",(U)q,n);
      free(q - HEAD);
      continue;
    }
    Work(q) = w;
    if (q == w) break;
    p = q;
  }
}
L P allocRaw(I n) {
  P p = (P) calloc(1, HEAD * sizeof(B) + n) + HEAD;
  Getter(p) = (I) intGet;
  Alloc(p) = n;
  Base(p) = p;
  return p;
}
L P allocTracked(P w, I n) {
  P p;
  Wu.i += HEAD * sizeof(B) + n;
  if (Wu.i >= Wh.i) {
    Z printf("GC used %ld garb %ld\n",Wu.i,Wh.i);
    gcMark(w);
    gcSweep(w,1);
    Wh.i = 2 * Wu.i;
    Z printf("   used %ld garb %ld\n",Wu.i,Wh.i);
  }
  p = allocRaw(n);
  Chain(p) = Chain(w); Chain(w) = p;
  Work(p) = w;
  w[Adr].p = p; /* prevent immediate gc */
  return p;
}
L P newVector(P w, I c) {
  I i, n = c * sizeof(B);
  P p = allocTracked(w, n);
  Getter(p) = (I) boxGet;
  Offset(p) = -1;
  for (i = Fin; i < c; ++i) p[i].p = Wz.p;
  return p;
}
L P newBuffer(P w, S b, I n) {
  P p = allocTracked(w,n);
  p[Fin].p = p[Tag].p = p[Adr].p = p[Dat].p = p[Len].p = Wz.p;
  if (b != 0) { memcpy(p, b, (size_t) n); Length(p) = n; }
  return p;
}
L P growBox(P p) {
  P w = Work(p), v = p;
  if (Base(p) == p) {
    if (Offset(p) == -1) { /* vector */
      I o = Limit(p), c = (3*o)/2+5;
      v = newVector(w,c);
      memcpy(v+Fin,p+Fin,(o-Fin)*sizeof(B));
    } else if (Alloc(p) != -1) { /* buffer */
      I o = Alloc(p), c = (3*o)/2+5;
      v = newBuffer(w,0,c);
      memcpy(v+Fin,p+Fin,o+(-Fin)*sizeof(B));
    }
    v[Len].p = Wz.p; /* force Len slot to be int, so hash can detect growth */
  }
  return v;
}
L P newMapped(P w, S b, I n) {
  P p = newVector(w,0);
  Alloc(p) = n; Base(p) = w; Offset(p) = b - (S) w;
  return p;
}
L P newShared(P w, P b, I n) {
  P p = newVector(w,0);
  I l = lenAt(*b);
  if (b->i + n > l) n = l - b->i;
  if (n < 0) n = 0;
  Length(p) = n;
  Base(p) = b->p; Offset(p) = strAt(*b) + b->i - (S) b->p;
  Getter(p) = (I) intGet;
  return p;
}
L I intPush(P p, I i) {
  P w = Work(p); B t;
  t.p = Wz.p; t.i = i;
  Push(p,t);
  return i;
}
L M strPush(P p, S s, I n) {
  P w = Work(p); B t;
  if (n == ~0)
    if (s == 0) {
      n = 0; t.p = Wn.p;
    } else {
      n = strlen(s); t.p = newBuffer(w,s,n+1);
    }
  else if (n >= 0 || s == 0)
    t.p = newBuffer(w,s,n);
  else {
    n = ~n; t.p = newMapped(w,s,n);
  }
  Length(t.p) = n;
  t.i = -1;
  Push(p,t);
  return (M) strAt(t);
}
L P boxPush(P p, I n) {
  P w = Work(p); B t;
  t.p = newVector(w,n); t.i = -1;
  Push(p,t);
  return t.p;
}
L V nilPush(P p, I n) {
  P w = Work(p);
  if (n < 0)
    Length(p) += n;
  else
    while (--n >= 0)
      Push(p,Wn);
}
L I makeRef(P p, I i) {
  P w = Work(p);
  I n = Wx.i;
  A(i < Limit(p));
  Wx.i = n < Length(Wx.p) ? Wx.p[n].i : ++Length(Wx.p);
  if (n >= Limit(Wx.p)) Wx.p = growBox(Wx.p);
  A(n < Limit(Wx.p));
  Wx.p[n] = p[i];
  return n;
}
L V pushRef(P p, I i) {
  P w = Work(p);
  Push(p,Wx.p[i]);
}
L V dropRef(P p, I i) {
  P w = Work(p);
  Wx.p[i].p = Wn.p; Wx.p[i].i = Wx.i;
  Wx.i = i;
}
L S cString(P p, I i) {
  P w = Work(p); I n;
  if (i < 0) i += Length(p);
  n = lenAt(p[i]);
  if (n >= Alloc(p[i].p) || strAt(p[i])[n] != 0) {
    S q = strAt(p[i]);
    p[i].p = newBuffer(w,0,n+2); Length(p[i].p) = n; p[i].i = -1;
    memcpy((M)p[i].p,q,(size_t)n);
  }
  return strAt(p[i]);
}
L I hashVal(B b) {
  Q x = (Q)b.i;
  if (!isInt(b)) {
    if (isVec(b) || b.i >= 0)
      x ^= (Q)(I)b.p; /* address also affects hash */
    else {
      S p = strAt(b); I i = lenAt(b);
      /* similar to Python's stringobject.c */
      x = *p << 7;
      while (--i >= 0) x = (1000003 * x) ^ *p++;
      x ^= lenAt(b);
    }
  }
  return x & 0x7fffffff;
}
L I hashFind(P p, I n, I f) {
  P v = p[Len].p;
  I t = isStr(p[n]), k = lenAt(p[n]);
  S s = strAt(p[n]);
  I z, h = hashVal(p[n]), m = v[Len].i-1, i = m&~h, o = m & (h^(h>>3));
  Q* q = (Q*)String(v);
  I c; D c = m;
  if (o == 0) o = m;
  for (;;) {
    i = m & (i+o);
    z = q[i];
    if (--z < 0) break;
    D --c; A(c >= 0);
    if ((!t && p[n].i == p[z].i && p[n].p == p[z].p) ||
        (t && k == lenAt(p[z]) && memcmp(s,strAt(p[z]),k) == 0))
      return z;
    o <<= 1;
    if (o > m) o ^= v[Tag].i;
  }
  if (f) q[i] = n+1;
  return -1;
}
L I log2next(I n) {
  I i = -1;
  if (n >= 0)
    for (;;) { ++i; if (n == 0) break; n >>= 1; }
  return i;
}
L V makeHash(P p, I c) {
  static C adj [] = {
    0, 0, 3, 3, 3, 5, 3, 3, 29, 17, 9, 5, 83, 27, 43, 3,
    45, 9, 39, 39, 9, 5, 3, 33, 27, 9, 71, 39, 9, 5, 83, 0
  };
  P w = Work(p);
  I i, n = log2next((3*Limit(p))/2);
  P v = newBuffer(w,0,(1<<n)*sizeof(Q));
  p[Len].p = v;
  v[Len].i = 1<<n;
  v[Tag].i = (1 << n) + adj[n];
  for (i = 0; i < c; ++i) hashFind(p,i,1);
}
L S scanSym(M* p) {
  C *q, *r = *p, s = '"';
  while (whsp(*r)) ++r;
  if (*r == 0) { *p = 0; return 0; }
  *p = q = r;
  switch (*r) {
    case '\'':
      s = 0;
    case '"':
      *q++ = *r++;
      while (*r != 0 && (s != 0 || !whsp(*r))) {
	if (*r == '\\')
	  switch (*++r) {
	    case 'f': *r = '\f'; break;
	    case 'n': *r = '\n'; break;
	    case 'r': *r = '\r'; break;
	    case 't': *r = '\t'; break;
	    case 'q': *r = '"'; break;
	    case 's': *r = ' '; break;
	  }
	*q = *r;
	if (*r++ == s) break;
	++q;
      }
      break;
    default:
      while (*r != 0 && !whsp(*r)) ++r;
      q = r;
  }
  if (*r != 0) ++r;
  *q = 0;
  q = *p;
  *p = r;
  Z printf("scan <%s>\n",q);
  return q;
}
/* safe variant of strtol: bounded, no whitespace, only decimal and hex */
L I strtoi(S p, I n, S *e) {
  I b = 10, v = 0, s = 1;
  *e = p + n;
  if (n > 0 && (*p == '+' || *p == '-') && *p++ == '-') s = -1;
  if (p+2 < *e && p[0] == '0' && p[1] == 'x') { b = 16; p += 2; }
  if (p < *e)
    for (;;) {
      C c = *p++;
      if (b == 16 && (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
	c += 9;
      else if (c < '0' || c > '9')
	break;
      v = b * v + (c & 0x0F);
      if (p == *e) return s*v;
    }
  *e = 0;
  return 0;
}
L V defSym(P w, S s, I n, P x, I y, I d) {
  I i = Wv.i;
  D if (Wq.i & 8) printf("defined %ld <%s> %lx/%ld\n",i,s,(U)x,y);
  Ws.p[i].p = newBuffer(w,0,n+1); Length(Ws.p[i].p) = n; Ws.p[i].i = -1;
  /* TODO rewrite using cString, need to avoid past-mem access by newBuffer */
  memcpy(Ws.p[i].p,s,n);
  Wv.p[i].p = x; Wv.p[i].i = y;
  Wi.p[i].i = i == 0;
  if (d) hashFind(Ws.p,Wv.i++,1);
}
L I findSym(P w) {
  I i; B x;
  S s, t = strAt(Wtopd); I n = lenAt(Wtopd);
  if (Wv.i >= Limit(Wv.p)) {
    Ws.p = growBox(Ws.p);
    Wv.p = growBox(Wv.p);
    Wi.p = growBox(Wi.p);
    makeHash(Ws.p,Wv.i);
  }
  Ws.p[Wv.i] = Wpopd;
  i = hashFind(Ws.p,Wv.i,0);
  if (i >= 0) return i;
  x.i = (I) strtoi(t,n,&s);
  if (s == t + n) x.p = Wz.p;
  else if (*t == '\'' || *t == '"') {
    x.p = newBuffer(w,t+1,n-1); Length(x.p) = n-1; x.i = -1;
  } else { x = Wc; Wl.i = Wv.i; }
  defSym(w,t,n,x.p,x.i,0);
  return Wv.i;
}
L V evalSym(P w, I i) {
  B* v = Wv.p + i;
  if (i == Wv.i && v->p == Wc.p) {
    hashFind(Ws.p,Wv.i++,1);
    if (Wt.i <= 0) Wv.p[i] = Wn;
    if (Wt.i == 0) { Wpushd.p = Wv.p; Wtopd.i = i; }
    if (Wt.i >= 0) return;
  } else if (Wt.i > 1) { /* redefinition */
    *v = Wc; Wi.p[i].i = 0; Wl.i = i; return;
  }
  if (Wt.i != 0 && Wi.p[i].i == 0) {
    Wc.p[Wc.i++] = *v;
  } else if (v->p == Wc.p) {
    w[--Wr.i] = Wp; Wp = *v;
  } else {
    Wc.p[0] = *v; w[--Wr.i] = Wp; Wp.p = Wc.p; Wp.i = 0;
  }
}
L S primitives = 
  "[[ exit work syst 0< 0<= 0> 0>= 0= 0<> int? vec? str? abs neg not 1+ "
  "1- 2+ 2- 2* 2/ b2p b2o b2i tget tcnt tstr tfin ttag tadr tdat tlen + "
  "- * div mod and or xor << >> box rbox < <= > >= = <> p= s= drop dup "
  "over swap nip tuck rot rrot r@ r> >r pick roll o( )o nvec nbuf nsha "
  "exec setg c@ i@ @ c! i! b! ! b@ 2drop 2dup 2over 2swap 2r@ 2r> 2>r "
  "(lit) ++ -- xini fgc vcpy cmp typ log2 csz (to) +to ljmp find car grow "
  "hshm hshf hshv s2i p2x i2x i2d rol rolq "
;
L I priGet(P p, I c) {
  P w = Work(p), t = w + Wd.i - 1;
  Z printf("primt @ %ld: %ld\n",Wp.i,c);
  /*%renumber< case >%*/
  switch (c) {
  /* special */
    case  0: /* [[   */	Wt.i = 0; break;
    case  1: /* exit */	Wp = w[Wr.i++];
			D if (Wr.i<Wd.i+10) {
			    printf("low stack! Wd %ld Wr %ld\n",Wd.i,Wr.i);
			    if (Wr.i<Wd.i+3) Wq.i |= 2;
			  }
			A(Getter(Wp.p) == Getter(Ge.p) || Wr.i == MAXWORK);
			break;
    case  2: /* work */	t->p = w; break;
    case  3: /* syst */	return 1;
  /* predicates */
    case  4: /* 0<   */	t->i = t->i < 0 ? 1 : 0; break;
    case  5: /* 0<=  */	t->i = t->i <= 0 ? 1 : 0; break;
    case  6: /* 0>   */	t->i = t->i > 0 ? 1 : 0; break;
    case  7: /* 0>=  */	t->i = t->i >= 0 ? 1 : 0; break;
    case  8: /* 0=   */	t->i = t->i == 0 ? 1 : 0; break;
    case  9: /* 0<>  */	t->i = t->i != 0 ? 1 : 0; break;
    case 10: /* int? */	t->i = isInt(*t) ? 1 : 0; break;
    case 11: /* vec? */	t->i = isVec(*t) ? 1 : 0; break;
    case 12: /* str? */	t->i = isStr(*t) ? 1 : 0; break;
  /* unary */
    case 13: /* abs  */	if (t->i > 0) break; /* else fall through */
    case 14: /* neg  */	t->i = - t->i; break;
    case 15: /* not  */	t->i = ~ t->i; break;
    case 16: /* 1+   */	t->i += 1; break;
    case 17: /* 1-   */	t->i -= 1; break;
    case 18: /* 2+   */	t->i += 2; break;
    case 19: /* 2-   */	t->i -= 2; break;
    case 20: /* 2*   */	t->i *= 2; break;
    case 21: /* 2/   */	t->i /= 2; break;
    case 22: /* b2p  */	t->i = 0; break;
    case 23: /* b2o  */	t->i = -1; break;
    case 24: /* b2i  */	t->p = Wz.p; break;
    case 25: /* tget */	*t = t->p[Get]; break;
    case 26: /* tcnt */	*t = t->p[Cnt]; break;
    case 27: /* tstr */	*t = t->p[Str]; break;
    case 28: /* tfin */	*t = t->p[Fin]; break;
    case 29: /* ttag */	*t = t->p[Tag]; break;
    case 30: /* tadr */	*t = t->p[Adr]; break;
    case 31: /* tdat */	*t = t->p[Dat]; break;
    case 32: /* tlen */	*t = t->p[Len]; break;
  /* binary */
    case 33: /* +    */	t[-1].i += Wpopd.i; break;
    case 34: /* -    */	t[-1].i -= Wpopd.i; break;
    case 35: /* *    */	t[-1].i *= Wpopd.i; break;
    case 36: /* div  */ t[-1].i /= Wpopd.i; break;
    case 37: /* mod  */ t[-1].i %= Wpopd.i; break;
    case 38: /* and  */ t[-1].i &= Wpopd.i; break;
    case 39: /* or   */	t[-1].i |= Wpopd.i; break;
    case 40: /* xor  */ t[-1].i ^= Wpopd.i; break;
    case 41: /* <<   */	t[-1].i <<= Wpopd.i; break;
    case 42: /* >>   */	t[-1].i = (U) t[-1].i >> Wpopd.i; break;
    case 43: /* box  */	t[-1].i = Wpopd.i; break;
    case 44: /* rbox */ t[-1].p = Wpopd.p; break;
  /* compare */
    case 45: /* <    */	t[-1].i = t[-1].i < Wpopd.i ? 1 : 0; break;
    case 46: /* <=   */	t[-1].i = t[-1].i <= Wpopd.i ? 1 : 0; break;
    case 47: /* >    */	t[-1].i = t[-1].i > Wpopd.i ? 1 : 0; break;
    case 48: /* >=   */	t[-1].i = t[-1].i >= Wpopd.i ? 1 : 0; break;
    case 49: /* =    */	t[-1].i = t[-1].i == Wpopd.i ? 1 : 0; break;
    case 50: /* <>   */	t[-1].i = t[-1].i != Wpopd.i ? 1 : 0; break;
    case 51: /* p=   */	t[-1].i = t[-1].p == Wpopd.p ? 1 : 0; break;
    case 52: /* s=   */	t[-1].i = lenAt(t[-1]) == lenAt(Wpopd) &&
			  memcmp(strAt(t[-1]),strAt(*t),lenAt(*t)) == 0 ? 1 : 0;
			break;
  /* stack */
    case 53: /* drop */ --Wd.i; break;
    case 54: /* dup  */	Wpushd = *t; break;
    case 55: /* over */ Wpushd = t[-1]; break;
    case 56: /* swap */ t[1] = *t; *t = t[-1]; t[-1] = t[1]; break;
    case 57: /* nip  */	t[-1] = Wpopd; break;
    case 58: /* tuck */ Wpushd = *t; *t = t[-1]; t[-1] = t[1]; break;
    case 59: /* rot  */	t[1]=t[-2]; t[-2]=t[-1]; t[-1]= *t; *t=t[1]; break;
    case 60: /* rrot */ t[1]= *t; *t=t[-1]; t[-1]=t[-2]; t[-2]=t[1]; break;
    case 61: /* r@   */	Wpushd = w[Wr.i]; break;
    case 62: /* r>   */	Wpushd = w[Wr.i++]; break;
    case 63: /* >r   */	w[--Wr.i] = Wpopd; break;
    case 64: /* pick */	*t = w[Wd.i-t->i-2]; break;
    case 65: /* roll */	{ I i = Wpopd.i+1; *t = w[Wd.i-i];
			  while (i-- >= 0) { w[Wd.i-i-1] = w[Wd.i-i]; } } break;
    case 66: /* o(   */	w[--Wr.i] = Wo; Wo = *t; break;
    case 67: /* )o   */	Wo = w[Wr.i++]; break;
  /* memory */
    case 68: /* nvec */	t->p = newVector(w,t->i); t->i = -1; break;
    case 69: /* nbuf */	t->p = newBuffer(w,0,t->i); t->i = -1; break;
    case 70: /* nsha */	t[-1].p = newShared(w,t-1,Wpopd.i); t[-1].i = -1; break;
    case 71: /* exec */	--Wd.i; return ((F)Getter(t->p))(t->p,t->i);
    case 72: /* setg */	Getter(t->p) = Getter(t[-1].p); Wd.i -= 2; break;
    case 73: /* c@   */	if (t->i >= 0) {
			  t->i = strAt(*t)[t->i]; t->p = Wz.p; break;
			} /* fall through */
    case 74: /* i@   */	if (t->i >= 0) {
			  t->i = ((Q*)strAt(*t))[t->i]; t->p = Wz.p; break;
			} /* fall through */
    case 75: /* @    */	*t = t->p[t->i]; break;
    case 76: /* c!   */	((M)strAt(*t))[t->i] = (C)t[-1].i; Wd.i -= 2; break;
    case 77: /* i!   */	((Q*)strAt(*t))[t->i] = t[-1].i; Wd.i -= 2; break;
    case 78: /* b!   */	(--t)->i = Wpopd.i; /* fall through */
    case 79: /* !    */	A(Fin <= t->i && t->i < Limit(t->p));
			t->p[t->i] = t[-1]; Wd.i -= 2; break;
    case 80: /* b@   */	t[-1] = t[-1].p[Wpopd.i]; break;
  /* duals */
    case 81: /* 2drop*/ Wd.i -= 2; break;
    case 82: /* 2dup */	Wpushd = t[-1]; Wpushd = *t; break;
    case 83: /* 2over*/ Wpushd = t[-3]; Wpushd = t[-2]; break;
    case 84: /* 2swap*/ t[1] = *t; *t = t[-2]; t[-2] = t[1];
			t[1] = t[-1]; t[-1] = t[-3]; t[-3] = t[1]; break;
    case 85: /* 2r@  */	Wpushd = w[Wr.i]; Wpushd = w[Wr.i+1]; break;
    case 86: /* 2r>  */	Wpushd = w[Wr.i++]; Wpushd = w[Wr.i++]; break;
    case 87: /* 2>r  */	w[--Wr.i] = Wpopd; w[--Wr.i] = Wpopd; break;
  /* other */
    case 88: /* lit  */	Wpushd = Wp.p[Wp.i++]; break;
    case 89: /* ++   */ ++(t->p[t->i].i); --Wd.i; break;
    case 90: /* --   */ --(t->p[t->i].i); --Wd.i; break;
    case 91: /* xini */	extInit(w); break;
    case 92: /* fgc  */	gcMark(w); gcSweep(w,1); break;
    case 93: /* vcpy */	while (--(t->i) >= 0)
			  t[-1].p[t[-1].i++] = t[-2].p[t[-2].i++];
			--Wd.i; break;
    case 94: /* cmp  */	if (t[-1].p == Wpopd.p)
			  t[-1].i = t[-1].i < t->i ? -1 : 
				    t[-1].i > t->i ? 1 : 0;
			else {
			  I m = lenAt(t[-1]), n = lenAt(*t);
			  I f = memcmp(strAt(t[-1]),strAt(*t),m<n?m:n);
			  t[-1].i = f<0 || (f==0 && m<n) ? -1 :
				    f>0 || (f==0 && m>n) ? 1 : 0;
			}
			break;
    case 95: /* typ  */	t->i = Type(*t); t->p = Wz.p; break;
    case 96: /* log2 */	t->i = log2next(t->i); break;
    case 97: /* csz  */ cString(w,-1); break;
    case 98: /* (to) */	w[Wp.p[Wp.i++].i] = Wpopd; break;
    case 99: /* +to  */	w[Wp.p[Wp.i++].i].i += Wpopd.i; break;
   case 100: /* ljmp */	Wp = Wpopd; break;
   case 101: /* find */	t->i = findSym(w); ++Wd.i; break;
   case 102: /* car  */	if (++(t->i) >= Length(t->p)) *t = t->p[Dat]; break;
   case 103: /* grow */	t->p = growBox(t->p); break;
   case 104: /* hshm */	makeHash(t->p,t->i); --Wd.i; break;
   case 105: /* hshf */	t[-1].i = hashFind(t[-1].p,t[-1].i,t->i); --Wd.i; break;
   case 106: /* hshv */	t->i = hashVal(*t); t->p = Wz.p; break;
   case 107: /* s2i  */ { S s; t->i = strtoi(strAt(*t),lenAt(*t),&s);
			  if (s != 0) t->p = Wz.p; break; }
   case 108: /* p2x  */ t->i = (I)t->p; /* fall hrough */
   case 109: /* i2x  */
   case 110: /* i2d  */ { C b[25]; I i = sprintf(b,(c==110?"%ld":"0x%lx"),t->i);
			  t->p = newBuffer(w,b,i); break; }
   case 111: /* rol  */ t[-1].i = (t[-1].i<<t->i)|((U)t[-1].i>>(BPI-t->i));
			--Wd.i; break;
   case 112: /* rolq */ t[-1].i = (I)(UQ)((t[-1].i<<t->i) |
					  ((UQ)t[-1].i>>(32-t->i)));
			--Wd.i; break;
    default: A(0);
  }
  return 0;
  /*%renumber<>%*/
}
L I exeGet(P p, I c) {
  P w = Work(p);
  w[--Wr.i] = Wp; Wp.p = p; Wp.i = c;
  return 0;
}
L I vofGet(P p, I c) {
  P w = Work(p);
  Wpushd = w[c];
  return 0;
}
L I jmpGet(P p, I c) {
  P w = Work(p);
  Wp.i += c;
  return 0;
}
L I cndGet(P p, I c) {
  P w = Work(p);
  if (Wpopd.i == 0) Wp.i += c;
  return 0;
}
L I tstGet(P p, I c) {
  P w = Work(p);
  if (Wtopd.i == 0) { Wpopd; Wp.i += c; }
  return 0;
}
L I usrGet(P p, I c) {
  P w = Work(p);
  /* printf("usrget %d\n",c); */
  Wpushd.p = p; Wtopd.i = c;
  return ((F)Getter(p[Adr].p))(p[Adr].p,p[Adr].i);
}
L I rawGet(P p, I c) {
  return ((F)(I)p)(p,c);
}
L P extCode(P w, S s, F f) {
  B t;
  t.p = newVector(w,0); t.i = 0;
  Getter(t.p) = (I) f;
  Push(Wg.p,t);
  if (s != 0) defSym(w,s,(I)strlen(s),Wg.p,Length(Wg.p)-1,1); 
  return t.p;
}
L I runWork(P w, I f) {
  I l = Limit(w);
  if (f) { w[--Wr.i] = Wp; Wp = We; }
  D if (Wq.i & 1)
      printf("RUN f %-2ld c %lx.%-4ld v %lx.%-4ld r %lx.%-5ld d %lx.%ld\n",
	    f,(U)Wc.p,Wc.i,(U)Wv.p,Wv.i,(U)Wr.p,Wr.i,(U)w,Wd.i);
  /*TODO get rid of the while condition, make sure stack has proper exit */
  while (Wr.i < l) {
    B* p = Wp.p + Wp.i++;
    /* collect code access statistics, but only for the Wc items */
    D if (Wp.i <= Wk.i && Wp.p == Wc.p) ++Wk.p[Wp.i-1].i;
#ifdef TDEBUG
    if (Wq.i & 2) {
      printf("    p %07lx.%-4ld r %3ld d %3ld ",(U)Wp.p,Wp.i,Wr.i,Wd.i);
      printf(" top %07lx.%-6ld", (U)Wtopd.p,Wtopd.i);
      if ((F)Getter(p->p) == priGet)
	printf("      %s\n",strAt(Ws.p[p->i]));
      else {
	int i = Wv.i;
	if (Wq.i & 4) {
	  for (i = 0; i < Wv.i; ++i)
	    if (Wv.p[i].i == p->i && Wv.p[i].p == p->p)
	      break;
	}
	if (i < Wv.i)
	  printf("    > %s\n",strAt(Ws.p[i]));
	else
	  printf(" exe: %07lx.%ld\n",(U)p->p,p->i);
      }
    }
#endif
    if (((F)Getter(p->p))(p->p,p->i)) {
      if (Wtopd.i == -1) Wp = w[Wr.i++];
      return Wpopd.i;
    }
  }
  return 0;
}
L V endWork(P w) {
  /* do not mark anything, therefore all boxes will be deleted */
  Alloc(Wz.p) = HEAD * sizeof(B);
  gcSweep(w, 0);
  A(Wu.i == 0);
  free(w - HEAD);
}
L V initBox(P w, P p, F g) {
  I i, c = Limit(p);
  Chain(p) = Chain(w); Chain(w) = p;
  Getter(p) = (I) g; Work(p) = w; Offset(p) = -1;
  for (i = Fin; i < c; ++i) p[i].p = Wz.p;
}
L V initRegs(P w, S s, C t, P p, I n) {
  C b[2]; b[0] = t;
  while (*s) { b[1] = *s++; defSym(w,b,2,p,n++,1); }
}
L P newWork(void) {
  P p;
  M b;
  L J j = {
    VERSION, newWork, runWork, endWork, intPush,
    strPush, boxPush, nilPush, makeRef, pushRef,
    dropRef, cString, scanSym, findSym, evalSym,
    extCode,
  };
  P w = allocRaw(MAXWORK * (I)sizeof(B));
  P c = allocRaw(MAXCODE * (I)sizeof(B));
  Wz.p = allocRaw(0);
  initBox(w,w,boxGet);
  initBox(w,c,exeGet);
  initBox(w,Wz.p,intGet);
  Alloc(Wz.p) = -1; Base(Wz.p) = w; Offset(Wz.p) = 0;
  Wd.i = MAXREGS + MAXVARS;
  Wr.i = MAXWORK;
  Wc.p = c;
  Wt.i = 1;
  Wu.i = (2 * HEAD + MAXCODE) * sizeof(B);
  Wh.i = 1000000000; /* prevent gc */
  Wn.p = newMapped(w,0,0);
  Ws.p = newVector(w,MAXSYMS);
  Wv.p = newVector(w,MAXSYMS);
  Wi.p = newVector(w,MAXSYMS);
  Wg.p = newVector(w,MAXGETF);
  Wx.p = newVector(w,50); /* grows as needed */
  Wj.p = newMapped(w,(S)&j,sizeof j);
  makeHash(Ws.p,0);
  makeRef(w,3); /* nil is the zero'th ref */
  Wn.p[Dat].p = newBuffer(w,0,0);
  Wn.p[Tag].i = -1;
  We.p = c; We.i = 2; /* default exception handler: exec -1 syst */
  /* order of these calls must match the G? defines */
  Push(Wg.p,Wz);
  extCode(w,0,boxGet);
  extCode(w,0,rawGet);
  extCode(w,0,priGet);
  extCode(w,0,exeGet);
  extCode(w,0,vofGet);
  extCode(w,0,jmpGet);
  extCode(w,0,cndGet);
  extCode(w,0,tstGet);
  extCode(w,0,usrGet);
  A(Length(Wg.p) == GMAX);
  Ww.p = Gv.p; Ww.i = MAXREGS;
  Z printf("Ww %lx/%ld\n",(U)Ww.p,Ww.i);
  Wh.i = 3 * Wu.i; /* enable gc */
  p = boxPush(w,10);
  strPush(p,NAME,~0);
  intPush(p,VERSION);
  strPush(p,__DATE__ " " __TIME__,~0);
  intPush(p,(I)sizeof(P));
  A(sizeof (B) == 2*sizeof (P));
  /* define the symbols of the primitives and the W? + G? registers */
  b = strPush(w,primitives,~0);
  for (;;) {
    S t = scanSym(&b);
    if (t == 0) break;
    defSym(w,t,strlen(t),Gp.p,Wv.i,1);
  }
  Wpopd;
  initRegs(w,WREGS,'W',Gv.p,-1);
  initRegs(w,GREGS,'G',Wg.p,0);
  return w;
}

#include "xbit.h"
#include "xdez.h"
#ifndef TNO_DYN
#include "xdyn.h"
#endif
#include "xfor.h"
#include "xmmf.h"
#include "xops.h"
#include "xpar.h"
#include "xstr.h"
#include "xtcl.h"
#include "xvec.h"

L V extInit(P w) {
  bitExt(extCode(w,"xbit",bitExt),-1); /* bit operations */
  dezExt(extCode(w,"xdez",dezExt),-1); /* zlib decompressor */
#ifndef TNO_DYN
  dynExt(extCode(w,"xdyn",dynExt),-1); /* dynamic extension support */
#endif
  forExt(extCode(w,"xfor",forExt),-1); /* resumable iterators */
  mmfExt(extCode(w,"xmmf",mmfExt),-1); /* memory-mapped files */
  opsExt(extCode(w,"xops",opsExt),-1); /* fast vector operations */
  parExt(extCode(w,"xpar",parExt),-1); /* tokenizer and parser */
  strExt(extCode(w,"xstr",strExt),-1); /* string operations */
  tclExt(extCode(w,"xtcl",tclExt),-1); /* Tcl token scanner/parser */
  vecExt(extCode(w,"xvec",vecExt),-1); /* 1..64-bit int vector access */
}
