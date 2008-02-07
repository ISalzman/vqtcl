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

static void *VecInsert (Vector *vecp, int off, int cnt) {
    Vector v = *vecp, v2;
    int unit = vType(v)->unit, limit = vLimit(v);
    char *cvec = (char*) v, *cvec2;
    assert(cnt > 0);
    assert(unit > 0);
    assert(vRefs(v) == 1);
    assert(off <= vCount(v));
    if (vCount(v) + cnt <= limit) {
        memmove(cvec+(off+cnt)*unit, cvec+off*unit, (vCount(v)-off)*unit);
        memset(cvec+off*unit, 0, cnt*unit);
    } else {
        limit += limit/2;
        if (limit < vCount(v) + cnt)
            limit = vCount(v) + cnt;
        v2 = vq_retain(AllocVector(vType(v), limit * unit));
        cvec2 = (char*) v2;
        vCount(v2) = vCount(v);
        vLimit(v2) = limit;
        memcpy(v2, v, off * unit);
        memcpy(cvec2+(off+cnt)*unit, cvec+off*unit, (vCount(v)-off)*unit);
        vCount(v) = 0; /* prevent cleanup of copied elements */
        vq_release(v);
        *vecp = v = v2;
    }
    vCount(v) += cnt;
    return v;
} 
static void *VecDelete (Vector *vecp, int off, int cnt) {
    /* TODO: shrink when less than half full and > 10 elements */
    Vector v = *vecp;
    int unit = vType(v)->unit;
    char *cvec = (char*) v;
    assert(cnt > 0);
    assert(unit > 0);
    assert(vRefs(v) == 1);
    assert(off + cnt <= vCount(v));
    memmove(cvec+off*unit, cvec+(off+cnt)*unit, (vCount(v)-(off+cnt))*unit);
    vCount(v) -= cnt;
    return v;
} 

static void *RangeFlip (Vector *vecp, int off, int count) {
    int pos, end, *iptr;
    if (*vecp == 0)
        *vecp = vq_retain(AllocDataVec(VQ_int, 4));
    if (count > 0) {
        end = vCount(*vecp);
        for (pos = 0, iptr = (int*) *vecp; pos < end; ++pos, ++iptr)
            if (off <= *iptr)
                break;
        if (pos < end && iptr[0] == off) {
            iptr[0] = off + count; /* extend at end */
            if (pos+1 < end && iptr[0] == iptr[1])
                VecDelete(vecp, pos, 2); /* merge with next */
        } else if (pos < end && iptr[0] == off + count) {
            iptr[0] = off; /* extend at start */
        } else {
            iptr = VecInsert(vecp, pos, 2); /* insert new range */
            iptr[pos] = off;
            iptr[pos+1] = off + count;
        }
    }
    return *vecp;
}
static int RangeLocate (Vector v, int off, int *offp) {
    int i, last = 0, fill = 0;
    const int *ivec = (const int*) v;
    for (i = 0; i < vCount(v) && off >= ivec[i]; ++i) {
        last = ivec[i];
        if (i & 1)
            fill += last - ivec[i-1];
    }
    if (--i & 1)
        fill = last - fill;
    *offp = fill + off - last;
    assert(*offp >= 0);
    return i;
}
static int RangeSpan (Vector v, int offset, int count, int *startp, int miss) {
    int rs, ps = RangeLocate(v, offset, &rs);
    int re, pe = RangeLocate(v, offset + count, &re);
    if ((ps ^ miss) & 1)
        rs = offset - rs;
    if ((pe ^ miss) & 1)
        re = offset + count - re;
    *startp = rs;
    return re - rs;
}
static int RangeExpand (Vector v, int off) {
    int i;
    const int *ivec = (const int*) v;
    for (i = 0; i < vCount(v) && ivec[i] <= off; i += 2)
        off += ivec[i+1] - ivec[i];
    return off;
}
static void RangeInsert (Vector *vecp, int off, int count, int mode) {
    Vector v = *vecp;
    int x, pos = RangeLocate(v, off, &x), miss = pos & 1, *ivec = (int*) v;
    assert(count > 0);
    while (++pos < vCount(v))
        ivec[pos] += count;
    if (mode == miss)
        RangeFlip(vecp, off, count);
} 
static void RangeDelete (Vector *vecp, int off, int count){
    int pos, pos2;
    int *ivec = (int*) *vecp;
    assert(count > 0);
    /* very tricky code because more than one range may have to be deleted */
    for (pos = 0; pos < vCount(*vecp); ++pos)
        if (ivec[pos] >= off) {
            for (pos2 = pos; pos2 < vCount(*vecp); ++pos2)
                if (ivec[pos2] > off + count)
                    break;
            if (pos & 1) {
                /* if both offsets are odd and the same, insert extra range */
                if (pos == pos2) {
                    pos2 += 2;
                    ivec = VecInsert(vecp, pos, 2);
                }
                ivec[pos] = off;
            }
            if (pos2 & 1)
                ivec[pos2-1] = off + count;
            /* if not both offsets are odd, then extend both to even offsets */
            if (!(pos & pos2 & 1)) {
                pos += pos & 1;
                pos2 -= pos2 & 1;
            }
            if (pos < pos2)
                ivec = VecDelete(vecp, pos, pos2-pos);
            while (pos < vCount(*vecp))
                ivec[pos++] -= count;
            break;
        }
}

static vq_Table IndirectTable (vq_Table meta, Dispatch *vtab, int extra) {
    int i, cols = vCount(meta);
    vq_Table t = vq_hold(AllocVector(vtab, cols * sizeof *t + extra));
    vMeta(t) = vq_retain(meta);
    vData(t) = t + cols;
    for (i = 0; i < cols; ++i) {
        t[i].o.a.m = t;
        t[i].o.b.i = i;
    }
    return t;
}

static vq_Type MutVecGetter (int row, vq_Item *item) {
    int col = item->o.b.i, aux = row;
    Vector v = item->o.a.m, *vecp = (Vector*) vData(v) + 3 * col;
    if (vecp[0] != 0 && RangeLocate(vecp[0], row, &aux) & 1) { /* translucent */
        if (vInsv(v) != 0 && (RangeLocate(vInsv(v), row, &row) & 1) == 0)
            return VQ_nil;
        if (vDelv(v) != 0)
            row = RangeExpand(vDelv(v), row);
        /* avoid t[col] dereference of vq_new-type tables, which have no rows */
        if (row < vCount(vOrig(v))) {
            *item = vOrig(v)[col];
            if (item->o.a.m != 0)
                return GetItem(row, item);
        }
    } else { /* opaque */
        if ((vecp[1] == 0 || (RangeLocate(vecp[1], aux, &aux) & 1) == 0)) {
            item->o.a.m = vecp[2];
            if (item->o.a.m != 0)
                return GetItem(aux, item);
        }
    }
    return VQ_nil;
}
static void InitMutVec (Vector v, int col) {
    Vector *vecp = (Vector*) vData(v) + 3 * col;
    if (vecp[0] == 0) {
        /* TODO: try to avoid allocating all vectors right away */
        vq_Type type = Vq_getInt(vMeta(v), col, 1, VQ_nil) & VQ_TYPEMASK;
        vecp[0] = vq_retain(AllocDataVec(VQ_int, 2));
        vecp[1] = vq_retain(AllocDataVec(VQ_int, 2));
        vecp[2] = vq_retain(AllocDataVec(type, 2));
    }
}
void MutVecSet (Vector v, int row, int col, const vq_Item *item) {
    int fill, miss;
    Vector *vecp = (Vector*) vData(v) + 3 * col;
    InitMutVec(v, col);
    if (row >= vCount(v))
        vCount(v) = row + 1;
    if (RangeLocate(vecp[0], row, &fill) & 1) {
        RangeFlip(vecp, row, 1);
        miss = RangeLocate(vecp[0], row, &row) & 1;
        assert(!miss);
        RangeInsert(vecp+1, row, 1, 0);
    } else
        row = fill;
    miss = RangeLocate(vecp[1], row, &fill) & 1;
    if (item != 0) { /* set */
        if (miss) { /* add new value */
            RangeFlip(vecp+1, row, 1);
            RangeLocate(vecp[1], row, &fill);
            VecInsert(vecp+2, fill, 1);
        }
        vType(vecp[2])->setter(vecp[2], fill, col, item);
    } else { /* unset */
        if (!miss) { /* remove existing value */
            vType(vecp[2])->setter(vecp[2], fill, col, 0);
            VecDelete(vecp+2, fill, 1);
            RangeFlip(vecp+1, row, 1);
        }
    }
}
void MutVecReplace (vq_Table t, int offset, int delrows, vq_Table data) {
    int c, r, cols = vCount(vMeta(t)), insrows = vCount(data);
    assert(offset >= 0 && delrows >= 0 && offset + delrows <= vCount(t));
    if (vInsv(t) == 0) {
        vInsv(t) = vq_retain(AllocDataVec(VQ_int, 2));
        vDelv(t) = vq_retain(AllocDataVec(VQ_int, 2));
    }
    vCount(t) += insrows;
    for (c = 0; c < cols; ++c) {
        vq_Type coltype = Vq_getInt(vMeta(t), c, 1, VQ_nil) & VQ_TYPEMASK;
        Vector *vecp = (Vector*) vData(t) + 3 * c;
        InitMutVec(t, c);
        if (delrows > 0) {
            int pos, len = RangeSpan(vecp[0], offset, delrows, &pos, 0);
            /* clear all entries to set up contiguous nil range */
            /* TODO: optimize and delete current values instead */
            for (r = 0; r < delrows; ++r)
                MutVecSet(t, offset + r, c, 0);
            if (len > 0)
                RangeDelete(vecp+1, pos, len);
            RangeDelete(vecp, offset, delrows);
        }
        if (insrows > 0) {
            RangeInsert(vecp, offset, insrows, 0); /* new translucent range */
            if (c < vCount(vMeta(data)))
                for (r = 0; r < insrows; ++r) {
                    vq_Item item = data[c];
                    if (GetItem(r, &item) == coltype)
                        MutVecSet(t, offset + r, c, &item);
                }
        }
    }
    vCount(t) -= delrows;
    if (delrows > 0) {
        int pos, len = RangeSpan(vInsv(t), offset, delrows, &pos, 1);
        if (len > 0)
            RangeInsert(&vDelv(t), pos, len, 1);
        RangeDelete(&vInsv(t), offset, delrows);
    }
    if (insrows > 0)
        RangeInsert(&vInsv(t), offset, insrows, 1);
}
static void MutVecCleaner (Vector v) {
    Vector *data = vData(v);
    int i = 3 * vCount(vMeta(v));
    while (--i >= 0)
        vq_release(data[i]);
    vq_release(vDelv(v));
    vq_release(vInsv(v));
    vq_release(vOrig(v));
    vq_release(vMeta(v));
    FreeVector(v);
}
static Dispatch muvtab = {
    "mutable", 4, sizeof(void*), 0, MutVecCleaner, MutVecGetter
};
int IsMutable (vq_Table t) {
    return vType(t) == &muvtab;
}
vq_Table WrapMutable (vq_Table t) {
    vq_Table m = vMeta(t);
    vq_Table w = IndirectTable(m, &muvtab, 3 * vCount(m) * sizeof(Vector));
    vCount(w) = vCount(t);
    vOrig(w) = vq_retain(t);
    return w;
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
        item.o.a.s = "name";   StringVecSetter (mm[0].o.a.m, 0, 0, &item);
        item.o.a.s = "type";   StringVecSetter (mm[0].o.a.m, 1, 0, &item);
        item.o.a.s = "subt";   StringVecSetter (mm[0].o.a.m, 2, 0, &item);
        item.o.a.i = VQ_string;   IntVecSetter (mm[1].o.a.m, 0, 0, &item);
        item.o.a.i = VQ_int;      IntVecSetter (mm[1].o.a.m, 1, 0, &item);
        item.o.a.i = VQ_table;    IntVecSetter (mm[1].o.a.m, 2, 0, &item);
        emt = vq_retain(vq_new(mm)); /* retain forever */
        item.o.a.m = emt;       TableVecSetter (mm[2].o.a.m, 0, 0, &item);
        item.o.a.m = emt;       TableVecSetter (mm[2].o.a.m, 1, 0, &item);
        item.o.a.m = mm; /*c*/  TableVecSetter (mm[2].o.a.m, 2, 0, &item);
    }
    return emt;
}

static vq_Type IotaVecGetter (int row, vq_Item *item) {
    item->o.a.i = row;
    return VQ_int;
}
static Dispatch iotatab = {
    "iota", 3, 0, 0, FreeVector, IotaVecGetter
};
vq_Table IotaTable (int rows, const char *name) {
    vq_Table t, meta = WrapMutable(vq_new(vq_meta(0)));
    Vq_setString(meta, 0, 0, name);
    Vq_setInt(meta, 0, 1, VQ_int);
    t = vq_retain(IndirectTable(meta, &iotatab, 0)); /* FIXME: extra retain */
    vCount(t) = rows;
    return t;
}
