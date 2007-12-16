/*  Vlerq core definitions and code.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "v_defs.h"

#pragma mark - MEMORY MANAGEMENT -

Vector AllocVector (Dispatch *vtab, int bytes) {
    Vector result;
    bytes += vtab->prefix * sizeof(vq_Item);
    result = (Vector) calloc(1, bytes) + vtab->prefix;
    vType(result) = vtab;
    return result;
}

void FreeVector (Vector v) {
    assert(vRefs(v) == 0);
    free(v - vType(v)->prefix);
}

#pragma mark - REFERENCE COUNTS -

Vector vq_retain (Vector v) {
    if (v != 0)
        ++vRefs(v); /* TODO: check for overflow */
    return v;
}

int vq_release (Vector v) {
    int n;
    if (v == 0)
        return -1;
    n = --vRefs(v);
    if (n <= 0) {
        /*assert(vRefs(v) == 0);*/
        if (vType(v)->cleaner != 0)
            vType(v)->cleaner(v);
        else
            FreeVector(v); /* TODO: when is this case needed? */
    }
    return n;
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
static vq_Type ViewVecGetter (int row, vq_Item *item) {
    vq_View *p = item->o.a.p;
    item->o.a.v = p[row];
    return VQ_view;
}
static vq_Type ObjRefVecGetter (int row, vq_Item *item) {
    VQ_ObjRef_t *p = item->o.a.p;
    item->o.a.r = p[row];
    return VQ_objref;
}

static void NilVecSetter (Vector v, int row, int col, const vq_Item *item) {
    char *p = (char*) v;
    int bit = 1 << (row&7);
    VQ_UNUSED(col);
    if (item != 0)
        p[row/8] |= bit;
    else
        p[row/8] &= ~bit;
}
static void IntVecSetter (Vector v, int row, int col, const vq_Item *item) {
    int *p = (int*) v;
    VQ_UNUSED(col);
    p[row] = item != 0 ? item->o.a.i : 0;
}
static void LongVecSetter (Vector v, int row, int col, const vq_Item *item) {
    int64_t *p = (int64_t*) v;
    VQ_UNUSED(col);
    p[row] = item != 0 ? item->w : 0;
}
static void FloatVecSetter (Vector v, int row, int col, const vq_Item *item) {
    float *p = (float*) v;
    VQ_UNUSED(col);
    p[row] = item != 0 ? item->o.a.f : 0;
}
static void DoubleVecSetter (Vector v, int row, int col, const vq_Item *item) {
    double *p = (double*) v;
    VQ_UNUSED(col);
    p[row] = item != 0 ? item->d : 0;
}
static void StringVecSetter (Vector v, int row, int col, const vq_Item *item) {
    const char **p = (const char**) v, *x = p[row];
    VQ_UNUSED(col);
    p[row] = item != 0 ? strcpy(malloc(strlen(item->o.a.s)+1), item->o.a.s) : 0;
    free((void*) x);
}
static void BytesVecSetter (Vector v, int row, int col, const vq_Item *item) {
    vq_Item *p = (vq_Item*) v, x = p[row];
    VQ_UNUSED(col);
    if (item != 0) {
        p[row].o.a.p = memcpy(malloc(item->o.b.i), item->o.a.p, item->o.b.i);
        p[row].o.b.i = item->o.b.i;
    } else {
        p[row].o.a.p = 0;
        p[row].o.b.i = 0;
    }
    free(x.o.a.p);
}
static void ViewVecSetter (Vector v, int row, int col, const vq_Item *item) {
    vq_View *p = (vq_View*) v, x = p[row];
    VQ_UNUSED(col);
    p[row] = item != 0 ? vq_retain(item->o.a.v) : 0;
    vq_release(x);
}
static void ObjRefVecSetter (Vector v, int row, int col, const vq_Item *item) {
    VQ_UNUSED(v);
    VQ_UNUSED(row);
    VQ_UNUSED(col);
    VQ_UNUSED(item);
/*
    Object_p *p = (Object_p*) v, x = p[row];
    VQ_UNUSED(col);
    p[row] = item != 0 ? ObjRetain(item->o.a.p) : 0;
    ObjRelease(x);
*/
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
static void ViewVecCleaner (Vector v) {
    int i;
    vq_View *p = (vq_View*) v;
    for (i = 0; i < vCount(v); ++i)
        vq_release(p[i]);
    FreeVector(v);
}
static void ObjRefVecCleaner (Vector v) {
    VQ_UNUSED(v);
/*
    int i;
    Object_p *p = (Object_p*) v;
    for (i = 0; i < vCount(v); ++i)
        ObjRelease(p[i]);
    FreeVector(v);
*/
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
    "viewvec", 2, sizeof(void*), 0, 
    ViewVecCleaner, ViewVecGetter, ViewVecSetter
};
static Dispatch ovtab = {
    "objrefvec", 2, sizeof(void*), 0, 
    ObjRefVecCleaner, ObjRefVecGetter, ObjRefVecSetter
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
        case VQ_view:
            vtab = &tvtab; bytes = rows * sizeof(vq_View); break;
        case VQ_objref:
            vtab = &ovtab; bytes = rows * sizeof(VQ_ObjRef_t); break;
        default: assert(0); return NULL;
    }
    v = AllocVector(vtab, bytes);
    vCount(v) = vLimit(v) = rows;
    return v;
}

#pragma mark - TABLE CREATION -

static void ViewCleaner (Vector v) {
    int i, n = vCount(vMeta(v));
    for (i = 0; i < n; ++i)
        vq_release(v[i].o.a.v);
    vq_release(vMeta(v));
    FreeVector(v);
}

/* TODO: support row replaces (and insert/delete) on standard views */
static Dispatch vtab = {
    "view", 2, sizeof(vq_Item), 0, ViewCleaner
};

vq_View vq_new (int rows, vq_View meta) {
    vq_View t;
    if (meta == 0)
        meta = EmptyMetaView();
    t = AllocVector(&vtab, vCount(meta) * sizeof(vq_Item));
    vCount(t) = rows;
    vMeta(t) = vq_retain(meta);
    if (rows > 0) {
        int i, n = vCount(meta);
        for (i = 0; i < n; ++i) {
            int type = Vq_getInt(meta, i, 1, VQ_nil) & VQ_TYPEMASK;
            t[i].o.a.v = vq_retain(AllocDataVec(type, rows));
        }
    }
    return t;
}

vq_View EmptyMetaView (void) {
    static vq_View meta = 0;
    if (meta == 0) {
        vq_View mm = AllocVector(&vtab, 3 * sizeof *mm);
        vMeta(mm) = mm; /* circular */
        vCount(mm) = 3;
        mm[0].o.a.v = vq_retain(AllocDataVec(VQ_string, 3));
        mm[1].o.a.v = vq_retain(AllocDataVec(VQ_int, 3));
        mm[2].o.a.v = vq_retain(AllocDataVec(VQ_view, 3));
        meta = vq_new(0, mm); /* retained forever */
        Vq_setMetaRow(mm, 0, "name", VQ_string, meta);
        Vq_setMetaRow(mm, 1, "type", VQ_int, meta);
        Vq_setMetaRow(mm, 2, "subv", VQ_view, meta);
    }
    return meta;
}

void IndirectCleaner (Vector v) {
    vq_release(vMeta(v));
    FreeVector(v);
}

vq_View IndirectView (vq_View meta, Dispatch *vtabp, int rows, int extra) {
    int i, cols = vCount(meta);
    vq_View t = AllocVector(vtabp, cols * sizeof *t + extra);
    assert(vtabp->prefix >= 3);
    vMeta(t) = vq_retain(meta);
    vData(t) = t + cols;
    vCount(t) = rows;
    for (i = 0; i < cols; ++i) {
        t[i].o.a.v = t;
        t[i].o.b.i = i;
    }
    return t;
}

#pragma mark - CORE TABLE FUNCTIONS -

vq_Type GetItem (int row, vq_Item *item) {
    Vector v = item->o.a.v;
    if (v == 0)
        return VQ_nil;
    assert(vType(v)->getter != 0);
    return vType(v)->getter(row, item);
}

vq_View vq_meta (vq_View t) {
    if (t == 0)
        t = EmptyMetaView();
    return vMeta(t);
}

int vq_size (vq_View t) {
    return t != 0 ? vCount(t) : 0;
}

int vq_empty (vq_View t, int row, int column) {
    vq_Item item;
    if (column < 0 || column >= vCount(vMeta(t)))
        return 1;
    item = t[column];
    return GetItem(row, &item) == VQ_nil;
}

vq_Item vq_get (vq_View t, int row, int column, vq_Type type, vq_Item def) {
    vq_Item item;
    VQ_UNUSED(type); /* TODO: is this really not used? */
    if (row < 0 || row >= vCount(t) || column < 0 || column >= vCount(vMeta(t)))
        return def;
    item = t[column];
    return GetItem(row, &item) != VQ_nil ? item : def;
}

void vq_set (vq_View t, int row, int col, vq_Type type, vq_Item val) {
    /* use view setter if defined, else column setter */
    if (vType(t)->setter == 0) {
        t = t[col].o.a.v;
        assert(t != 0 && vType(t)->setter != 0);
    }
    /* TODO: copy-on-write of columns if refcount > 1 */
    vType(t)->setter(t, row, col, type != VQ_nil ? &val : 0);
}

void vq_replace (vq_View t, int start, int count, vq_View data) {
    assert(start >= 0 && count >= 0 && start + count <= vCount(t));
    assert(t != 0 && vType(t)->replacer != 0);
    vType(t)->replacer(t, start, count, data);
}

#pragma mark - UTILITY WRAPPERS -

int Vq_getInt (vq_View t, int row, int col, int def) {
    vq_Item item;
    item.o.a.i = def;
    item = vq_get(t, row, col, VQ_int, item);
    return item.o.a.i;
}

const char *Vq_getString (vq_View t, int row, int col, const char *def) {
    vq_Item item;
    item.o.a.s = def;
    item = vq_get(t, row, col, VQ_string, item);
    return item.o.a.s;
}

vq_View Vq_getView (vq_View t, int row, int col, vq_View def) {
    vq_Item item;
    item.o.a.v = def;
    item = vq_get(t, row, col, VQ_view, item);
    return item.o.a.v;
}

void Vq_setEmpty (vq_View t, int row, int col) {
    vq_Item item;
    vq_set(t, row, col, VQ_nil, item);
}

void Vq_setInt (vq_View t, int row, int col, int val) {
    vq_Item item;
    item.o.a.i = val;
    vq_set(t, row, col, VQ_int, item);
}

void Vq_setString (vq_View t, int row, int col, const char *val) {
    vq_Item item;
    item.o.a.s = val;
    vq_set(t, row, col, val != 0 ? VQ_string : VQ_nil, item);
}

void Vq_setView (vq_View t, int row, int col, vq_View val) {
    vq_Item item;
    item.o.a.v = val;
    vq_set(t, row, col, val != 0 ? VQ_view : VQ_nil, item);
}

void Vq_setMetaRow (vq_View m, int row, const char *nam, int typ, vq_View sub) {
    Vq_setString(m, row, 0, nam);
    Vq_setInt(m, row, 1, typ);
    Vq_setView(m, row, 2, sub != NULL ? sub : EmptyMetaView());
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

#pragma mark - OPERATOR DISPATCH -

#if 0
CmdDispatch f_commands[] = {
    { "at",         "O:VIIO",   AtCmd_VIIO      },
    { "config",     "S:",       ConfigCmd_      },
    { "data",       "V:VIX",    DataCmd_VIX     },
    { "empty",      "I:VII",    EmptyCmd_VII    },
    { "iota",       "V:VS",     IotaCmd_VS      },
    { "mdef",       "V:O",      MdefCmd_O       },
    { "meta",       "V:V",      MetaCmd_V       },
    { "new",        "V:V",      NewCmd_V        },
    { "pass",       "V:V",      PassCmd_V       },
    { "size",       "V:V",      SizeCmd_V       },
#if VQ_LOAD_H
    { "desc2meta",  "V:S",      Desc2MetaCmd_S  },
    { "load",       "V:O",      LoadCmd_O       },
    { "open",       "V:S",      OpenCmd_S       },
#endif
#if VQ_MUTABLE_H
    { "replace",    "N:OIIV",   ReplaceCmd_OIIV },
    { "set",        "N:OIIO",   SetCmd_OIIO     },
    { "unset",      "N:OII",    UnsetCmd_OII    },
#endif
#if VQ_RANGES_H
    { "rflip",      "O:OII",    RflipCmd_OII    },
    { "rlocate",    "O:OI",     RlocateCmd_OI   },
    { "rinsert",    "O:OIII",   RinsertCmd_OIII },
    { "rdelete",    "O:OII",    RdeleteCmd_OII  },
#endif
#if VQ_SAVE_H
    { "emit",       "B:V",      EmitCmd_V       },
    { "meta2desc",  "O:V",      Meta2DescCmd_V  },
#endif
    { 0, 0, 0 }
};  
#endif
