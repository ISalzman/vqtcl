/* t4ext.h -- thrive public extension api, auto-extracted from thrive.h */
#define t4_V void

typedef char t4_C; typedef t4_C* t4_M; typedef const t4_C* t4_S;
typedef unsigned char t4_UC; typedef t4_UC* t4_UM; typedef const t4_UC* t4_US;
typedef short t4_H; typedef unsigned short t4_UH;
typedef int t4_Q; typedef unsigned int t4_UQ;
#if __INT_MAX__ < __LONG_MAX__
  typedef long t4_W;
#elif defined(__LONG_LONG_MAX__)
  typedef long long t4_W;
#elif defined(t4_WIN32)
  typedef __int64 t4_W;
#else
#include <limits.h>
#if t4_INT_MAX < t4_LONG_MAX
  typedef long t4_W;
#elif defined(t4_LLONG_MAX)
  typedef long long t4_W;
#endif
#endif
typedef long t4_I; typedef unsigned long t4_U;
typedef float t4_R; typedef double t4_E;
typedef struct t4_B { t4_I i; struct t4_B* p; } t4_B, *t4_P;
typedef t4_I (*t4_F)(t4_P,t4_I);

typedef struct t4_J {
  t4_I t4_f_VERSION;
  t4_P (*t4_f_newWork)(void);
  t4_I (*t4_f_runWork)(t4_P,t4_I);
  t4_V (*t4_f_endWork)(t4_P);
  t4_I (*t4_f_intPush)(t4_P,t4_I);
  t4_M (*t4_f_strPush)(t4_P,t4_S,t4_I);
  t4_P (*t4_f_boxPush)(t4_P,t4_I);
  t4_V (*t4_f_nilPush)(t4_P,t4_I);
  t4_I (*t4_f_makeRef)(t4_P,t4_I);
  t4_V (*t4_f_pushRef)(t4_P,t4_I);
  t4_V (*t4_f_dropRef)(t4_P,t4_I);
  t4_S (*t4_f_cString)(t4_P,t4_I);
  t4_S (*t4_f_scanSym)(t4_M*);
  t4_I (*t4_f_findSym)(t4_P);
  t4_V (*t4_f_evalSym)(t4_P,t4_I);
  t4_P (*t4_f_extCode)(t4_P,t4_S,t4_F);
} t4_J;

#define t4_HEAD 8
enum {
  t4_Get = -t4_HEAD, t4_Cnt, t4_Str, t4_Fin, t4_Tag, t4_Adr, t4_Dat, t4_Len
};
enum {
  t4_Tn, t4_Ti, t4_Ts, t4_Td, t4_Ta, t4_To, t4_Te
};

#define t4_Chain(x)  ((x)[t4_Get].p)
#define t4_Getter(x) ((x)[t4_Get].i)
#define t4_Work(x)   ((x)[t4_Cnt].p)
#define t4_Alloc(x)  ((x)[t4_Cnt].i)
#define t4_Limit(x)  (t4_Alloc(x)/(t4_I)sizeof(t4_B))
#define t4_Base(x)   ((x)[t4_Str].p)
#define t4_Offset(x) ((x)[t4_Str].i)
#define t4_String(x) ((t4_S)t4_Base(x)+t4_Offset(x)) /* evals x twice */
#define t4_Length(x) ((x)[t4_Len].i)

#define t4_strAt(x) t4_String((x).p) /* evals x twice */
#define t4_lenAt(x) t4_Length((x).p)
#define t4_isInt(x) (t4_Alloc((x).p)==-1)
#define t4_isVec(x) (t4_Offset((x).p)==-1)
#define t4_isStr(x) (!t4_isInt(x)&&!t4_isVec(x)) /* evals x twice */

#define t4_Type(x) \
  (t4_isInt(x) ? t4_Ti : \
    ((x).i != -1 || t4_Getter((x).p) == t4_Getter(t4_Wz.p) \
      ? (t4_isVec(x) ? t4_To : t4_strAt(x) != 0 ? t4_Ts : t4_Tn) \
      : (t4_isVec(x) ? t4_Ta : t4_strAt(x) != 0 ? t4_Td : t4_Te)))

#define t4_VERSION	t4_jVec->t4_f_VERSION
#define t4_newWork	t4_jVec->t4_f_newWork
#define t4_runWork	t4_jVec->t4_f_runWork
#define t4_endWork	t4_jVec->t4_f_endWork
#define t4_intPush	t4_jVec->t4_f_intPush
#define t4_strPush	t4_jVec->t4_f_strPush
#define t4_boxPush	t4_jVec->t4_f_boxPush
#define t4_nilPush	t4_jVec->t4_f_nilPush
#define t4_makeRef	t4_jVec->t4_f_makeRef
#define t4_pushRef	t4_jVec->t4_f_pushRef
#define t4_dropRef	t4_jVec->t4_f_dropRef
#define t4_cString	t4_jVec->t4_f_cString
#define t4_scanSym	t4_jVec->t4_f_scanSym
#define t4_findSym	t4_jVec->t4_f_findSym
#define t4_evalSym	t4_jVec->t4_f_evalSym
#define t4_extCode	t4_jVec->t4_f_extCode

#define t4_DEFINE(n) t4_J* t4_jVec; t4_I n
#define t4_extIni(x) t4_jVec = (t4_J*) t4_strAt(x[16]/*t4_Wj*/)

extern t4_J* t4_jVec;
/* t4ext.h -- end */
