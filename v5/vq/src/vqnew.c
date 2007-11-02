/* Vlerq creation of tables with settable content */

#include "vqdefs.h"

#include <stdlib.h>
#include <string.h>

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

static Vector AllocDataVec (vq_Type type, int rows) {
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

static void TableCleaner (Vector v) {
    vq_release(vMeta(v));
    FreeVector(v);
}
static Dispatch vtab = { "table", 2, sizeof(vq_Item), 0, TableCleaner };

vq_Table vq_new (vq_Table meta) {
    vq_Table t;
    if (meta == 0)
        meta = EmptyMetaTable();
    t = vq_hold(AllocVector(&vtab, 0));
    vMeta(t) = vq_retain(meta);
    return t;
}

vq_Table EmptyMetaTable (void) {
    static vq_Table emt = 0;
    if (emt == 0) {
        vq_Item item;
        vq_Table mm = AllocVector(&vtab, 3 * sizeof *mm);
        vMeta(mm) = vq_retain(mm); /* circular */
        vCount(mm) = 3;
        /* can't use normal set calls because they insist on a mutable table */
        mm[0].o.a.m = vq_retain(AllocDataVec(VQ_string, 3));
        mm[1].o.a.m = vq_retain(AllocDataVec(VQ_int, 3));
        mm[2].o.a.m = vq_retain(AllocDataVec(VQ_table, 3));
        vCount(mm[0].o.a.m) = 3;
        vCount(mm[1].o.a.m) = 3;
        vCount(mm[2].o.a.m) = 3;
        item.o.a.s = "name";   StringVecSetter(mm[0].o.a.m, 0, 0, &item);
        item.o.a.s = "type";   StringVecSetter(mm[0].o.a.m, 1, 0, &item);
        item.o.a.s = "subt";   StringVecSetter(mm[0].o.a.m, 2, 0, &item);
        item.o.a.i = VQ_string;   IntVecSetter(mm[1].o.a.m, 0, 0, &item);
        item.o.a.i = VQ_int;      IntVecSetter(mm[1].o.a.m, 1, 0, &item);
        item.o.a.i = VQ_table;    IntVecSetter(mm[1].o.a.m, 2, 0, &item);
        emt = vq_retain(vq_new(mm)); /* retain forever */
        item.o.a.m = emt;       TableVecSetter(mm[2].o.a.m, 0, 0, &item);
        item.o.a.m = emt;       TableVecSetter(mm[2].o.a.m, 1, 0, &item);
        item.o.a.m = mm; /*c*/  TableVecSetter(mm[2].o.a.m, 2, 0, &item);
    }
    return emt;
}
