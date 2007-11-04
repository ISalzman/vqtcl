/* Vlerq core */

#include "defs.h"

#include <stdlib.h>
#include <string.h>

#define POOL_CHUNK 64

/* TODO: make globals thread-safe */
static vq_Pool currentPool;

#pragma mark - MEMORY MANAGEMENT -

Vector AllocVector (Dispatch *vtab, int bytes) {
    Vector result;
    bytes += vtab->prefix * sizeof(vq_Item);
    result = (Vector) calloc(1, bytes) + vtab->prefix;
    vType(result) = vtab;
    return result;
}

void FreeVector (Vector v) {
    free(v - vType(v)->prefix);
}

#pragma mark - MEMORY POOLS -

static vq_Pool NewPoolChunk (vq_Pool prev) {
    vq_Pool pool = malloc(POOL_CHUNK * sizeof(void*));
    pool[0] = (void*) prev;
    pool[1] = (void*) (pool + 2);
    return pool;
}
static void RestorePool (void *p) {
    currentPool = p;
}
vq_Pool vq_addpool (void) {
    vq_Pool prevPool = currentPool;
    currentPool = NewPoolChunk(0);
    vq_holdf(prevPool, &RestorePool);
    return currentPool;
}
void vq_losepool (vq_Pool pool) {
    /* TODO: deal with pool != currentPool */
    do {
        vq_Pool next = (void*) pool[0];
        void **rover = (void*) pool[1];
        while (*--rover >= (void*) (pool + 2))
            vq_release(*rover);
        free(pool);
        pool = next;
    } while (pool != 0);
}
Vector vq_hold (Vector v) {
    void **rover;
    if ((void*) currentPool[1] >= (void*) (currentPool + POOL_CHUNK))
        currentPool = NewPoolChunk(currentPool);
    rover = (void*) currentPool[1];
    *rover++ = vq_retain(v);
    currentPool[1] = (void*) rover;
    return v;
}
static void HoldCleaner (Vector v) {
    v[0].o.a.c(v[0].o.b.p);
    FreeVector(v);
}
Vector vq_holdf (void *p, void (*f)(void*)) {
    static Dispatch vtab = { "hold", 1, 0, 0, HoldCleaner };
    Vector v = vq_hold(AllocVector(&vtab, sizeof *v));
    v[0].o.a.c = f;
    v[0].o.b.p = p;
    return v;
}
/* the above code doesn't need to know anything about reference counting */
Vector vq_retain (Vector v) {
    if (v != 0)
        ++vRefs(v); /* TODO: check for overflow */
    return v;
}
void vq_release (Vector v) {
    if (v != 0 && --vRefs(v) <= 0) {
        /*assert(vRefs(v) == 0);*/
        if (vType(v)->cleaner != 0)
            vType(v)->cleaner(v);
        else
            FreeVector(v); /* TODO: when is this case needed? */
    }
}

#pragma mark - DATA VECTORS -

static vq_Type NilVecGetter (int row, vq_Item *item) {
    const char *p = item->o.a.p;
    /* item->o.a.i = (p[row/8] >> (row&7)) & 1; */
    return (p[row/8] >> (row&7)) & 1 ? VQ_int : VQ_nil;
}
static vq_Type IntVecGetter (int row, vq_Item *item) {
    const int *p = item->o.a.p;
    item->o.a.i = p[row];
    return VQ_int;
}
static vq_Type LongVecGetter (int row, vq_Item *item) {
    const int64_t *p = item->o.a.p;
    item->w = p[row];
    return VQ_long;
}
static vq_Type FloatVecGetter (int row, vq_Item *item) {
    const float *p = item->o.a.p;
    item->o.a.f = p[row];
    return VQ_float;
}
static vq_Type DoubleVecGetter (int row, vq_Item *item) {
    const double *p = item->o.a.p;
    item->d = p[row];
    return VQ_double;
}
static vq_Type StringVecGetter (int row, vq_Item *item) {
    const char **p = item->o.a.p;
    item->o.a.s = p[row] != 0 ? p[row] : "";
    return VQ_string;
}
static vq_Type BytesVecGetter (int row, vq_Item *item) {
    vq_Item *p = item->o.a.p;
    *item = p[row];
    return VQ_bytes;
}
static vq_Type TableVecGetter (int row, vq_Item *item) {
    vq_Table *p = item->o.a.p;
    item->o.a.m = p[row];
    return VQ_table;
}
static vq_Type ObjectVecGetter (int row, vq_Item *item) {
    Object_p *p = item->o.a.p;
    item->o.a.p = p[row];
    return VQ_object;
}

static void NilVecSetter (Vector v, int row, int col, const vq_Item *item) {
    char *p = (char*) v;
    int bit = 1 << (row&7);
    if (item != 0)
        p[row/8] |= bit;
    else
        p[row/8] &= ~bit;
}
static void IntVecSetter (Vector v, int row, int col, const vq_Item *item) {
    int *p = (int*) v;
    p[row] = item != 0 ? item->o.a.i : 0;
}
static void LongVecSetter (Vector v, int row, int col, const vq_Item *item) {
    int64_t *p = (int64_t*) v;
    p[row] = item != 0 ? item->w : 0;
}
static void FloatVecSetter (Vector v, int row, int col, const vq_Item *item) {
    float *p = (float*) v;
    p[row] = item != 0 ? item->o.a.f : 0;
}
static void DoubleVecSetter (Vector v, int row, int col, const vq_Item *item) {
    double *p = (double*) v;
    p[row] = item != 0 ? item->d : 0;
}
static void StringVecSetter (Vector v, int row, int col, const vq_Item *item) {
    const char **p = (const char**) v, *x = p[row];
    p[row] = item != 0 ? strcpy(malloc(strlen(item->o.a.s)+1), item->o.a.s) : 0;
    free((void*) x);
}
static void BytesVecSetter (Vector v, int row, int col, const vq_Item *item) {
    vq_Item *p = (vq_Item*) v, x = p[row];
    if (item != 0) {
        p[row].o.a.p = memcpy(malloc(item->o.b.i), item->o.a.p, item->o.b.i);
        p[row].o.b.i = item->o.b.i;
    } else {
        p[row].o.a.p = 0;
        p[row].o.b.i = 0;
    }
    free(x.o.a.p);
}
static void TableVecSetter (Vector v, int row, int col, const vq_Item *item) {
    vq_Table *p = (vq_Table*) v, x = p[row];
    p[row] = item != 0 ? vq_retain(item->o.a.m) : 0;
    vq_release(x);
}
static void ObjectVecSetter (Vector v, int row, int col, const vq_Item *item) {
    Object_p *p = (Object_p*) v, x = p[row];
    p[row] = item != 0 ? ObjIncRef(item->o.a.p) : 0;
    ObjDecRef(x);
}

static void StringVecCleaner (Vector v) {
    int i;
    const char **p = (const char**) v;
    for (i = 0; i < vCount(v); ++i)
        free((void*) p[i]);
    FreeVector(v);
}
static void BytesVecCleaner (Vector v) {
    int i;
    vq_Item *p = (vq_Item*) v;
    for (i = 0; i < vCount(v); ++i)
        free(p[i].o.a.p);
    FreeVector(v);
}
static void TableVecCleaner (Vector v) {
    int i;
    vq_Table *p = (vq_Table*) v;
    for (i = 0; i < vCount(v); ++i)
        vq_release(p[i]);
    FreeVector(v);
}
static void ObjectVecCleaner (Vector v) {
    int i;
    Object_p *p = (Object_p*) v;
    for (i = 0; i < vCount(v); ++i)
        ObjDecRef(p[i]);
    FreeVector(v);
}

static Dispatch nvtab = {
    "nilvec", 2, -1, 0, FreeVector, NilVecGetter, NilVecSetter 
};
static Dispatch ivtab = {
    "intvec", 2, 4, 0, FreeVector, IntVecGetter, IntVecSetter 
};
static Dispatch lvtab = {
    "longvec", 2, 8, 0, FreeVector, LongVecGetter, LongVecSetter  
};
static Dispatch fvtab = {
    "floatvec", 2, 4, 0, FreeVector, FloatVecGetter, FloatVecSetter  
};
static Dispatch dvtab = {
    "doublevec", 2, 8, 0, FreeVector, DoubleVecGetter, DoubleVecSetter  
};
static Dispatch svtab = {
    "stringvec", 2, sizeof(void*), 0, 
    StringVecCleaner, StringVecGetter, StringVecSetter
};
static Dispatch bvtab = {
    "bytesvec", 2, sizeof(vq_Item), 0, 
    BytesVecCleaner, BytesVecGetter, BytesVecSetter
};
static Dispatch tvtab = {
    "tablevec", 2, sizeof(void*), 0, 
    TableVecCleaner, TableVecGetter, TableVecSetter
};
static Dispatch ovtab = {
    "objectvec", 2, sizeof(void*), 0, 
    ObjectVecCleaner, ObjectVecGetter, ObjectVecSetter
};

Vector AllocDataVec (vq_Type type, int rows) {
    int bytes;
    Vector v;
    Dispatch *vtab;
    switch (type) {
        case VQ_nil:
            vtab = &nvtab; bytes = ((rows+31)/32) * sizeof(int); break;
        case VQ_int:
            vtab = &ivtab; bytes = rows * sizeof(int); break;
        case VQ_long:
            vtab = &lvtab; bytes = rows * sizeof(int64_t); break;
        case VQ_float:
            vtab = &fvtab; bytes = rows * sizeof(float); break;
        case VQ_double:
            vtab = &dvtab; bytes = rows * sizeof(double); break;
        case VQ_string:
            vtab = &svtab; bytes = rows * sizeof(const char*); break;
        case VQ_bytes:
            vtab = &bvtab; bytes = rows * sizeof(vq_Item); break;
        case VQ_table:
            vtab = &tvtab; bytes = rows * sizeof(vq_Table); break;
        case VQ_object:
            vtab = &ovtab; bytes = rows * sizeof(Object_p); break;
        default: assert(0);
    }
    v = AllocVector(vtab, bytes);
    vLimit(v) = rows;
    return v;
}

#pragma mark - TABLE CREATION -

static void TableCleaner (Vector v) {
    vq_release(vMeta(v));
    FreeVector(v);
}
static Dispatch vtab = { "table", 2, sizeof(vq_Item), 0, TableCleaner };

vq_Table vq_new (vq_Table meta, int bytes) {
    vq_Table t;
    if (meta == 0)
        meta = EmptyMetaTable();
    t = vq_hold(AllocVector(&vtab, bytes));
    vMeta(t) = vq_retain(meta);
    return t;
}

vq_Table EmptyMetaTable (void) {
    static vq_Table meta = 0;
    if (meta == 0) {
        vq_Item item;
        vq_Table mm = AllocVector(&vtab, 3 * sizeof *mm);
        vMeta(mm) = vq_retain(mm); /* circular */
        vCount(mm) = 3;
        mm[0].o.a.m = vq_retain(AllocDataVec(VQ_string, 3));
        mm[1].o.a.m = vq_retain(AllocDataVec(VQ_int, 3));
        mm[2].o.a.m = vq_retain(AllocDataVec(VQ_table, 3));
        vCount(mm[0].o.a.m) = 3;
        vCount(mm[1].o.a.m) = 3;
        vCount(mm[2].o.a.m) = 3;
        item.o.a.s = "name";    StringVecSetter(mm[0].o.a.m, 0, 0, &item);
        item.o.a.s = "type";    StringVecSetter(mm[0].o.a.m, 1, 0, &item);
        item.o.a.s = "subt";    StringVecSetter(mm[0].o.a.m, 2, 0, &item);
        item.o.a.i = VQ_string; IntVecSetter(mm[1].o.a.m, 0, 0, &item);
        item.o.a.i = VQ_int;    IntVecSetter(mm[1].o.a.m, 1, 0, &item);
        item.o.a.i = VQ_table;  IntVecSetter(mm[1].o.a.m, 2, 0, &item);
        meta = vq_retain(vq_new(mm, 0)); /* circular, retained forever */
        item.o.a.m = meta;      TableVecSetter(mm[2].o.a.m, 0, 0, &item);
        item.o.a.m = meta;      TableVecSetter(mm[2].o.a.m, 1, 0, &item);
        item.o.a.m = mm; /*c*/  TableVecSetter(mm[2].o.a.m, 2, 0, &item);
    }
    return meta;
}

#pragma mark - CORE TABLE FUNCTIONS -

vq_Type GetItem (int row, vq_Item *item) {
    Vector v = item->o.a.m;
    assert(v != 0 && vType(v)->getter != 0);
    return vType(v)->getter(row, item);
}

vq_Table vq_meta (vq_Table t) {
    if (t == 0)
        t = EmptyMetaTable();
    return vMeta(t);
}
int vq_size (vq_Table t) {
    return t != 0 ? vCount(t) : 0;
}
vq_Item vq_get (vq_Table t, int row, int column, vq_Type type, vq_Item def) {
    vq_Item item;
    if (row < 0 || row >= vCount(t) || column < 0 || column >= vCount(vMeta(t)))
        return def;
    item = t[column];
    return GetItem(row, &item) != VQ_nil ? item : def;
}
void vq_set (vq_Table t, int row, int col, vq_Type type, vq_Item val) {
    Vector v = t[col].o.a.m;
    assert(vType(v)->setter != 0);
    vType(v)->setter(v, row, col, &val);
}

#pragma mark - UTILITY WRAPPERS -

int Vq_getInt (vq_Table t, int row, int col, int def) {
    vq_Item item;
    item.o.a.i = def;
    item = vq_get(t, row, col, VQ_int, item);
    return item.o.a.i;
}
const char *Vq_getString (vq_Table t, int row, int col, const char *def) {
    vq_Item item;
    item.o.a.s = def;
    item = vq_get(t, row, col, VQ_string, item);
    return item.o.a.s;
}
vq_Table Vq_getTable (vq_Table t, int row, int col, vq_Table def) {
    vq_Item item;
    item.o.a.m = def;
    item = vq_get(t, row, col, VQ_table, item);
    return item.o.a.m;
}

void Vq_setInt (vq_Table t, int row, int col, int val) {
    vq_Item item;
    item.o.a.i = val;
    vq_set(t, row, col, VQ_int, item);
}
void Vq_setString (vq_Table t, int row, int col, const char *val) {
    vq_Item item;
    item.o.a.s = val;
    vq_set(t, row, col, val != 0 ? VQ_string : VQ_nil, item);
}
void Vq_setTable (vq_Table t, int row, int col, vq_Table val) {
    vq_Item item;
    item.o.a.m = val;
    vq_set(t, row, col, val != 0 ? VQ_table : VQ_nil, item);
}

#pragma mark - TYPE DESCRIPTORS -

int CharAsType (char c) {
    const char *p = strchr(VQ_TYPES, c & ~0x20);
    int type = p != 0 ? p - VQ_TYPES : VQ_nil;
    if (c & 0x20)
        type |= VQ_NULLABLE;
    return type;
}
int StringAsType (const char *str) {
    int type = CharAsType(*str);
    while (*++str != 0)
        if ('a' <= *str && *str <= 'z')
        type |= 1 << (*str - 'a' + 5);
    return type;
}
const char* TypeAsString (int type, char *buf) {
    char c, *p = buf; /* buffer should have room for at least 28 bytes */
    *p++ = VQ_TYPES[type&VQ_TYPEMASK];
    if (type & VQ_NULLABLE)
        p[-1] |= 0x20;
    for (c = 'a'; c <= 'z'; ++c)
        if (type & (1 << (c - 'a' + 5)))
            *p++ = c;
    *p = 0;
    return buf;
}

#pragma mark - OPERATOR WRAPPERS -

static vq_Type AtCmd_TIIO (vq_Item a[]) {
    vq_Table t = a[0].o.a.m;
    vq_Type type = Vq_getInt(vMeta(t), a[2].o.a.i, 1, VQ_nil) & VQ_TYPEMASK;
    if (type != VQ_nil && ObjToItem(type, a+3)) {
        *a = vq_get(t, a[1].o.a.i, a[2].o.a.i, type, a[3]);
        return type;
    }
    *a = a[3];
    return VQ_object;
}
static vq_Type MetaCmd_T (vq_Item a[]) {
    a->o.a.m = vq_meta(a[0].o.a.m);
    return VQ_table;
}
static vq_Type NewCmd_T (vq_Item a[]) {
    a->o.a.m = vq_new(a[0].o.a.m, 0);
    return VQ_table;
}
static vq_Type SizeCmd_T (vq_Item a[]) {
    vq_Table t = vq_new(0, 0);
    vCount(t) = vq_size(a[0].o.a.m);
    a->o.a.m = t;
    return VQ_table;
}
static vq_Type VersionCmd_ (vq_Item a[]) {
    a->o.a.s = VQ_VERSION;
    return VQ_string;
}

#if VQ_MOD_MKLOAD
static vq_Type Desc2MetaCmd_S (vq_Item a[]) {
    a->o.a.m = DescToMeta(a[0].o.a.s, -1);
    return VQ_table;
}
static vq_Type OpenCmd_S (vq_Item a[]) {
    Vector map = OpenMappedFile(a[0].o.a.s);
    if (map == 0)
        return VQ_nil;
    a->o.a.m = MapToTable(map);
    return VQ_table;
}
#endif

#pragma mark - OPERATOR DISPATCH -

CmdDispatch f_commands[] = {
    { "at",         "O:TIIO",   AtCmd_TIIO      },
    { "meta",       "T:T",      MetaCmd_T       },
    { "new",        "T:T",      NewCmd_T        },
    { "size",       "T:T",      SizeCmd_T       },
    { "version",    "S:",       VersionCmd_     },
#if VQ_MOD_MKLOAD
    { "desc2meta",  "T:S",      Desc2MetaCmd_S  },
    { "open",       "T:S",      OpenCmd_S       },
#endif
    { 0, 0, 0 }
};  
