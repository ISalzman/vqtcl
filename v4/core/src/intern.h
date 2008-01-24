/*
 * intern.h - Internal definitions.
 */

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef ptrdiff_t Int_t; /* large enough to hold a pointer */

typedef unsigned char Byte_t;

#ifdef OBJECT_TYPE
typedef OBJECT_TYPE Object_p;
#else
#include "defs.h"
#endif

#if !defined(VALUES_MUST_BE_ALIGNED) && (defined(__sparc__) || defined(__sgi__))
#define VALUES_MUST_BE_ALIGNED 1
#endif

#if !defined(_BIG_ENDIAN) && defined(WORDS_BIGENDIAN)
#define _BIG_ENDIAN 1
#endif

#if DEBUG+0
#define Assert(x) do if (!(x)) FailedAssert(#x, __FILE__, __LINE__); while (0)
#define DbgIf(x)  if (GetShared()->debug && (x))
#else
#define Assert(x)
#define DbgIf(x)  if (0)
#endif

#define DBG_trace   (1<<0)
#define DBG_deps    (1<<1)

#define PUSH_KEEP_REFS                            \
  { Shared_p _shared = GetShared();               \
    struct Buffer _keep, *_prev = _shared->keeps; \
    InitBuffer(_shared->keeps = &_keep);
    
#define POP_KEEP_REFS         \
    ReleaseSequences(&_keep); \
    _shared->keeps = _prev;   \
  }

/* keep in sync with CharAsItemType() in column.c */
typedef enum ItemTypes {
    IT_unknown,
    IT_int,
    IT_wide,
    IT_float,
    IT_double,
    IT_string,
    IT_bytes,
    IT_object,
    IT_column,
    IT_view,
    IT_error
} ItemTypes;

typedef enum ErrorCodes {
    EC_cioor,
    EC_rioor,
    EC_cizwv,
    EC_nmcw,
    EC_rambe,
    EC_nalor,
    EC_wnoa
} ErrorCodes;

struct Shared {
    struct SharedInfo *info;   /* ext_tcl.c */
    int                refs;   /* ext_tcl.c */
    int                debug;  /* debug flags */
    struct Buffer     *keeps;  /* column.c */
    struct Sequence   *empty;  /* view.c */
};

typedef struct Shared   *Shared_p;
typedef struct SeqType  *SeqType_p;
typedef struct Sequence *Seq_p;
typedef struct Column   *Column_p;
typedef union Item      *Item_p;
typedef Seq_p            View_p;

typedef ItemTypes  (*Getter) (int row, Item_p item);
typedef void       (*Cleaner) (Seq_p seq);

struct SeqType {
  const char *name;      /* for introspection only */
  Getter      getfun;    /* item accessor, copied to Sequence during setup */
  short       cleanups;  /* octal for data[0..3].p: 0=none, 1=decref, 2=free */
  Cleaner     cleaner;   /* called before performing the above cleanups */
};

struct Sequence {
  int        count;   /* number of entries */
  int        refs;    /* reference count */
  SeqType_p  type;    /* type dispatch table */
  Getter     getter;  /* item accessor function */
  union { int i; void *p; Seq_p q; Object_p o; } data[4]; /* getter-specific */
};

#define IncRefCount(seq) AdjustSeqRefs(seq, 1)
#define DecRefCount(seq) AdjustSeqRefs(seq, -1)
#define KeepRef(seq) IncRefCount(LoseRef(seq))

typedef struct Column {
    Seq_p        seq;    /* the sequence which will handle accesses */
    int          pos;    /* position of this column in the view */
} Column;              
                       
typedef union Item {
    char         b [8];  /* used for raw access and byte-swapping */
    int          q [2];  /* used for hash calculation of wides and doubles */
    int          i;      /* IT_int */
    int64_t      w;      /* IT_wide */
    float        f;      /* IT_float */
    double       d;      /* IT_double */
    const void  *p;      /* used to convert pointers */
    const char  *s;      /* IT_string */
    struct { const Byte_t *ptr; int len; } u; /* IT_bytes */
    Object_p     o;      /* IT_object */
    Column       c;      /* IT_column */
    View_p       v;      /* IT_view */
    ErrorCodes   e;      /* IT_error */
} Item;

/* column.c */

void      *(AdjustSeqRefs) (void *refs, int count);
void       (AppendToStrVec) (const void *s, int bytes, Seq_p seq);
int        (CastObjToItem) (char type, Item_p item);
ItemTypes  (CharAsItemType) (char type);
Column     (CoerceColumn) (ItemTypes type, Object_p obj);
void       (FailedAssert) (const char *msg, const char *file, int line);
Column     (ForceStringColumn) (Column column);
Item       (GetColItem) (int row, Column column, ItemTypes type);
ItemTypes  (GetItem) (int row, Item_p item);
Item       (GetSeqItem) (int row, Seq_p seq, ItemTypes type);
Item       (GetViewItem) (View_p view, int row, int col, ItemTypes type);
Seq_p      (LoseRef) (Seq_p seq);
Seq_p      (NewBitVec) (int count);
Seq_p      (NewIntVec) (int count, int **dataptr);
Seq_p      (NewPtrVec) (int count, Int_t **dataptr);
Seq_p      (NewSequence) (int count, SeqType_p type, int auxbytes);
Seq_p      (NewSequenceNoRef) (int count, SeqType_p type, int auxbytes);
Seq_p      (NewSeqVec) (ItemTypes type, const Seq_p *p, int n);
Seq_p      (NewStrVec) (int istext);
Seq_p      (FinishStrVec) (Seq_p seq);
void       (ReleaseSequences) (struct Buffer *keep);
Seq_p      (ResizeSeq) (Seq_p seq, int pos, int diff, int elemsize);
Column     (SeqAsCol) (Seq_p seq);

/* buffer.c */

typedef struct Buffer *Buffer_p;
typedef struct Overflow *Overflow_p;

struct Buffer {
    union { char *c; int *i; const void **p; } fill;
    char       *limit;
    Overflow_p  head;
    Int_t       saved;
    Int_t       used;
    char       *ofill;
    char       *result;
    char        buf [128];
    char        slack [8];
};

#define ADD_ONEC_TO_BUF(b,x) (*(b).fill.c++ = (x))

#define ADD_CHAR_TO_BUF(b,x) \
          { char _c = (x); \
            if ((b).fill.c < (b).limit) *(b).fill.c++ = _c; \
              else AddToBuffer(&(b), &_c, sizeof _c); }

#define ADD_INT_TO_BUF(b,x) \
          { int _i = (x); \
            if ((b).fill.c < (b).limit) *(b).fill.i++ = _i; \
              else AddToBuffer(&(b), &_i, sizeof _i); }

#define ADD_PTR_TO_BUF(b,x) \
          { const void *_p = (x); \
            if ((b).fill.c < (b).limit) *(b).fill.p++ = _p; \
              else AddToBuffer(&(b), &_p, sizeof _p); }

#define BufferFill(b) ((b)->saved + ((b)->fill.c - (b)->buf))

void   (InitBuffer) (Buffer_p bp);
void   (ReleaseBuffer) (Buffer_p bp, int keep);
void   (AddToBuffer) (Buffer_p bp, const void *data, Int_t len);
void  *(BufferAsPtr) (Buffer_p bp, int fast);
Seq_p  (BufferAsIntVec) (Buffer_p bp);
int    (NextBuffer) (Buffer_p bp, char **firstp, int *countp);

/* view.c */

typedef enum MetaCols {
  MC_name, MC_type, MC_subv, MC_limit
} MetaCols;

#define V_Cols(view) ((Column_p) ((Seq_p) (view) + 1))
#define V_Types(view) (*(char**) ((view)->data))
#define V_Meta(view) (*(View_p*) ((view)->data+1))

#define ViewAsCol(v) SeqAsCol(v)
#define ViewWidth(view) ((view)->count)
#define ViewCol(view,col) (V_Cols(view)[col])
#define ViewColType(view,col) ((ItemTypes) V_Types(view)[col])
#define ViewCompat(view1,view2) MetaIsCompatible(V_Meta(view1), V_Meta(view2))

View_p  (DescAsMeta) (const char** desc, const char* end);
View_p  (EmptyMetaView) (void);
char    (GetColType) (View_p meta, int col);
View_p  (IndirectView) (View_p meta, Seq_p seq);
View_p  (NewView) (View_p meta);
View_p  (MakeIntColMeta) (const char *name);
int     (MetaIsCompatible) (View_p meta1, View_p meta2);
void    (ReleaseUnusedViews) (void);
void    (SetViewCols) (View_p view, int first, int count, Column src);
void    (SetViewSeqs) (View_p view, int first, int count, Seq_p src);

/* hash.c */

int    (HashMapAdd) (Seq_p hmap, int key, int value);
int    (HashMapLookup) (Seq_p hmap, int key, int defval);
Seq_p  (HashMapNew) (void);
int    (HashMapRemove) (Seq_p hmap, int key);

/* file.c */

typedef Seq_p MappedFile_p;

MappedFile_p  (InitMappedFile) (const char *data, Int_t length, Cleaner fun);
View_p        (MappedToView) (MappedFile_p map, View_p base);
View_p        (OpenDataFile) (const char *filename);

/* emit.c */

typedef void *(*SaveInitFun)(void*,Int_t);
typedef void *(*SaveDataFun)(void*,const void*,Int_t);

void   (MetaAsDesc) (View_p meta, Buffer_p buffer);
Int_t  (ViewSave) (View_p view, void *aux, SaveInitFun, SaveDataFun, int);

/* getters.c */

SeqType_p (PickIntGetter) (int bits);
SeqType_p (FixedGetter) (int bytes, int rows, int real, int flip);

/* bits.c */
      
char  *(Bits2elias) (const char *bytes, int count, int *outsize);
void   (ClearBit) (Seq_p bitmap, int row);
int    (CountBits) (Seq_p seq);
Seq_p  (MinBitCount) (Seq_p *pbitmap, int count);
int    (NextBits) (Seq_p bits, int *fromp, int *countp);
int    (NextElias) (const char *bytes, int count, int *inbits);
int    (SetBit) (Seq_p *pbitmap, int row);
void   (SetBitRange) (Seq_p bits, int from, int count);
int    (TestBit) (Seq_p bitmap, int row);
int    (TopBit) (int v);

/* mutable.c */

enum MutPrepares {
    MP_insdat, MP_adjdat, MP_usemap, MP_delmap, MP_adjmap,
    MP_insmap, MP_revmap, MP_limit
};

int     (IsMutable) (View_p view);
View_p  (MutableView) (View_p view);
Seq_p   (MutPrepare) (View_p view);
View_p  (ViewSet) (View_p view, int row, int col, Item_p item);

/* the following must be supplied in the language binding */

int            (ColumnByName) (View_p meta, Object_p obj);
struct Shared *(GetShared) (void);
void           (InvalidateView) (Object_p obj);
Object_p       (ItemAsObj) (ItemTypes type, Item_p item);
Object_p       (NeedMutable) (Object_p obj);
Column         (ObjAsColumn) (Object_p obj);
View_p         (ObjAsView) (Object_p obj);
int            (ObjToItem) (ItemTypes type, Item_p item);
