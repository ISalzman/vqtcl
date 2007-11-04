/* Support for mutable tables */

#include "defs.h"

#pragma mark - RANGE OPERATIONS -

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

#pragma mark - MUTABLE TABLE -

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
static void MutVecSetter (Vector v, int row, int col, const vq_Item *item) {
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
static void MutVecReplacer (vq_Table t, int offset, int delrows, vq_Table data) {
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
                MutVecSetter(t, offset + r, c, 0);
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
                        MutVecSetter(t, offset + r, c, &item);
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
    "mutable", 4, sizeof(void*), 0,
        MutVecCleaner, MutVecGetter, MutVecSetter, MutVecReplacer
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
