/* vlerq.h - this is the single-source-file header of Vlerq for Ruby */

/* %includeDir<../src>% */
/* %includeDir<../src_ruby>% */

/* %include<defs.h>% */
/*
 * defs.h - Binding-specific definitions needed by the general code.
 */

typedef unsigned long Object_p;
/* %include<intern.h>% */
/*
 * intern.h - Internal definitions.
 */

#include <stddef.h>
#include <unistd.h>

typedef ptrdiff_t Int_t; /* large enough to hold a pointer */

#ifdef OBJECT_TYPE
typedef OBJECT_TYPE Object_p;
#else
#endif

#ifdef __sparc__
#define VALUES_MUST_BE_ALIGNED 1
#endif

#ifndef __sparc__
typedef unsigned char uint8_t;
#endif

#if WORDS_BIGENDIAN && !defined(BIG_ENDIAN)
#define _BIG_ENDIAN 1
#endif

#if DEBUG
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
  struct { const uint8_t *ptr; int len; } u; /* IT_bytes */
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
/* %include<wrap_gen.h>% */
/* wrap_gen.h - generated code, do not edit */

View_p (BlockedView) (View_p);
View_p (CloneView) (View_p);
Column (CoerceCmd) (Object_p, const char*);
View_p (ColMapView) (View_p, Column);
View_p (ColOmitView) (View_p, Column);
View_p (ConcatView) (View_p, View_p);
View_p (DescToMeta) (const char*);
View_p (FirstView) (View_p, int);
Column (GetHashInfo) (View_p, View_p, int);
View_p (GroupCol) (View_p, Column, const char*);
View_p (GroupedView) (View_p, Column, Column, const char*);
int    (HashDoFind) (View_p, int, View_p, Column, Column, Column);
Column (HashValues) (View_p);
View_p (IjoinView) (View_p, View_p);
Column (IntersectMap) (View_p, View_p);
View_p (JoinView) (View_p, View_p, const char*);
View_p (LastView) (View_p, int);
Column (NewCountsColumn) (Column);
Column (NewIotaColumn) (int);
View_p (NoColumnView) (int);
View_p (ObjAsMetaView) (Object_p);
Column (OmitColumn) (Column, int);
View_p (OneColView) (View_p, int);
View_p (PairView) (View_p, View_p);
View_p (RemapSubview) (View_p, Column, int, int);
int    (RowCompare) (View_p, int, View_p, int);
int    (RowEqual) (View_p, int, View_p, int);
int    (RowHash) (View_p, int);
Column (SortMap) (View_p);
View_p (StepView) (View_p, int, int, int, int);
int    (StringLookup) (const char*, Column);
View_p (TagView) (View_p, const char*);
View_p (TakeView) (View_p, int);
View_p (UngroupView) (View_p, int);
Column (UniqMap) (View_p);
View_p (V_Meta) (View_p);
Column (ViewAsCol) (View_p);
Column (ViewCol) (View_p, int);
int    (ViewCompare) (View_p, View_p);
int    (ViewCompat) (View_p, View_p);
View_p (ViewReplace) (View_p, int, int, View_p);
int    (ViewSize) (View_p);
int    (ViewWidth) (View_p);

ItemTypes (AppendCmd_VV) (Item_p a);
ItemTypes (AtCmd_VIO) (Item_p a);
ItemTypes (AtRowCmd_OI) (Item_p a);
ItemTypes (BitRunsCmd_i) (Item_p a);
ItemTypes (BlockedCmd_V) (Item_p a);
ItemTypes (CloneCmd_V) (Item_p a);
ItemTypes (CoerceCmd_OS) (Item_p a);
ItemTypes (ColConvCmd_C) (Item_p a);
ItemTypes (ColMapCmd_Vn) (Item_p a);
ItemTypes (ColOmitCmd_Vn) (Item_p a);
ItemTypes (CompareCmd_VV) (Item_p a);
ItemTypes (CompatCmd_VV) (Item_p a);
ItemTypes (ConcatCmd_VV) (Item_p a);
ItemTypes (CountsCmd_VN) (Item_p a);
ItemTypes (CountsColCmd_C) (Item_p a);
ItemTypes (CountViewCmd_I) (Item_p a);
ItemTypes (DataCmd_VX) (Item_p a);
ItemTypes (DebugCmd_I) (Item_p a);
ItemTypes (DefCmd_OO) (Item_p a);
ItemTypes (DeleteCmd_VII) (Item_p a);
ItemTypes (DepsCmd_O) (Item_p a);
ItemTypes (EmitCmd_V) (Item_p a);
ItemTypes (EmitModsCmd_V) (Item_p a);
ItemTypes (ExceptCmd_VV) (Item_p a);
ItemTypes (ExceptMapCmd_VV) (Item_p a);
ItemTypes (FirstCmd_VI) (Item_p a);
ItemTypes (GetCmd_VX) (Item_p a);
ItemTypes (GetColCmd_VN) (Item_p a);
ItemTypes (GetInfoCmd_VVI) (Item_p a);
ItemTypes (GroupCmd_VnS) (Item_p a);
ItemTypes (GroupedCmd_ViiS) (Item_p a);
ItemTypes (HashColCmd_SO) (Item_p a);
ItemTypes (HashFindCmd_VIViii) (Item_p a);
ItemTypes (HashViewCmd_V) (Item_p a);
ItemTypes (IjoinCmd_VV) (Item_p a);
ItemTypes (InsertCmd_VIV) (Item_p a);
ItemTypes (IntersectCmd_VV) (Item_p a);
ItemTypes (IotaCmd_I) (Item_p a);
ItemTypes (IsectMapCmd_VV) (Item_p a);
ItemTypes (JoinCmd_VVS) (Item_p a);
ItemTypes (LastCmd_VI) (Item_p a);
ItemTypes (LoadCmd_O) (Item_p a);
ItemTypes (LoadModsCmd_OV) (Item_p a);
ItemTypes (LoopCmd_X) (Item_p a);
ItemTypes (MaxCmd_VN) (Item_p a);
ItemTypes (MdefCmd_O) (Item_p a);
ItemTypes (MdescCmd_S) (Item_p a);
ItemTypes (MetaCmd_V) (Item_p a);
ItemTypes (MinCmd_VN) (Item_p a);
ItemTypes (MutInfoCmd_V) (Item_p a);
ItemTypes (NameColCmd_V) (Item_p a);
ItemTypes (NamesCmd_V) (Item_p a);
ItemTypes (OmitMapCmd_iI) (Item_p a);
ItemTypes (OneColCmd_VN) (Item_p a);
ItemTypes (OpenCmd_S) (Item_p a);
ItemTypes (PairCmd_VV) (Item_p a);
ItemTypes (ProductCmd_VV) (Item_p a);
ItemTypes (RefCmd_OX) (Item_p a);
ItemTypes (RefsCmd_O) (Item_p a);
ItemTypes (RemapCmd_Vi) (Item_p a);
ItemTypes (RemapSubCmd_ViII) (Item_p a);
ItemTypes (RenameCmd_VO) (Item_p a);
ItemTypes (RepeatCmd_VI) (Item_p a);
ItemTypes (ReplaceCmd_VIIV) (Item_p a);
ItemTypes (ResizeColCmd_iII) (Item_p a);
ItemTypes (ReverseCmd_V) (Item_p a);
ItemTypes (RowCmpCmd_VIVI) (Item_p a);
ItemTypes (RowEqCmd_VIVI) (Item_p a);
ItemTypes (RowHashCmd_VI) (Item_p a);
ItemTypes (SaveCmd_VS) (Item_p a);
ItemTypes (SetCmd_MIX) (Item_p a);
ItemTypes (SizeCmd_V) (Item_p a);
ItemTypes (SliceCmd_VIII) (Item_p a);
ItemTypes (SortCmd_V) (Item_p a);
ItemTypes (SortMapCmd_V) (Item_p a);
ItemTypes (SpreadCmd_VI) (Item_p a);
ItemTypes (StepCmd_VIIII) (Item_p a);
ItemTypes (StrLookupCmd_Ss) (Item_p a);
ItemTypes (StructDescCmd_V) (Item_p a);
ItemTypes (StructureCmd_V) (Item_p a);
ItemTypes (SumCmd_VN) (Item_p a);
ItemTypes (TagCmd_VS) (Item_p a);
ItemTypes (TakeCmd_VI) (Item_p a);
ItemTypes (ToCmd_OO) (Item_p a);
ItemTypes (TypeCmd_O) (Item_p a);
ItemTypes (TypesCmd_V) (Item_p a);
ItemTypes (UngroupCmd_VN) (Item_p a);
ItemTypes (UnionCmd_VV) (Item_p a);
ItemTypes (UnionMapCmd_VV) (Item_p a);
ItemTypes (UniqueCmd_V) (Item_p a);
ItemTypes (UniqueMapCmd_V) (Item_p a);
ItemTypes (ViewCmd_X) (Item_p a);
ItemTypes (ViewAsColCmd_V) (Item_p a);
ItemTypes (ViewConvCmd_V) (Item_p a);
ItemTypes (WidthCmd_V) (Item_p a);
ItemTypes (WriteCmd_VO) (Item_p a);

/* end of generated code */

/* end of generated code */
