/*  Support for mutable views.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

typedef struct MutVec_s {
    vqVec insv, delv;
    vqView orig;
    int *perm;
} *MutVec;
                    
#define vInsv(v)   ((MutVec) vwAuxP(v))->insv
#define vDelv(v)   ((MutVec) vwAuxP(v))->delv
#define vOrig(v)   ((MutVec) vwAuxP(v))->orig
#define vPerm(v)   ((MutVec) vwAuxP(v))->perm

/* --------------------------------------------------- RANGE OPERATIONS ----- */

static int RangeSpan (vqVec v, int offset, int count, int *startp, int miss) {
    int rs, ps = RangeLocate(v, offset, &rs);
    int re, pe = RangeLocate(v, offset + count, &re);
    if ((ps ^ miss) & 1)
        rs = offset - rs;
    if ((pe ^ miss) & 1)
        re = offset + count - re;
    *startp = rs;
    return re - rs;
}

static int RangeExpand (vqVec v, int off) {
    int i;
    const int *ivec = (const int*) v;
    for (i = 0; i < vSize(v) && ivec[i] <= off; i += 2)
        off += ivec[i+1] - ivec[i];
    return off;
}

/* ------------------------------------------------------ MUTABLE TABLE ----- */

static vqType MutVecGetter (int row, vqCell *cp) {
    int col = cp->x.y.i, aux = row;
    vqView v = cp->v;
    vqVec *vecp = (vqVec*) vwMore(v) + 3 * col;
    if (vecp[0] != 0 && RangeLocate(vecp[0], row, &aux) & 1) { /* translucent */
        if (vInsv(v) != 0 && (RangeLocate(vInsv(v), row, &row) & 1) == 0)
            return VQ_nil;
        if (vDelv(v) != 0)
            row = RangeExpand(vDelv(v), row);
        /* avoid t[col] dereference of vq_new-type views, which have no rows */
        if (row < vwRows(vOrig(v))) {
            *cp = vwCol(vOrig(v),col);
            if (cp->v != 0)
                return getcell(row, cp);
        }
    } else { /* opaque */
        if ((vecp[1] == 0 || (RangeLocate(vecp[1], aux, &aux) & 1) == 0)) {
            cp->p = vecp[2];
            if (cp->p != 0)
                return getcell(aux, cp);
        }
    }
    return VQ_nil;
}

static void InitMutVec (vqView v, int col) {
    vqVec *vecp = (vqVec*) vwMore(v) + 3 * col;
    if (vecp[0] == 0) {
        /* TODO: try to avoid allocating all vectors right away */
        vqType type = vq_getInt(vwMeta(v), col, 1, VQ_nil) & VQ_TYPEMASK;
        vecp[0] = vq_incref(new_datavec(VQ_int, 2));
        vecp[1] = vq_incref(new_datavec(VQ_int, 2));
        vecp[2] = vq_incref(new_datavec(type, 2));
        vSize(vecp[0]) = 0;
        vSize(vecp[1]) = 0;
        vSize(vecp[2]) = 0;
    }
    /* sneaky: make sure getter always goes through mutvec from now on */
    if (vwCol(v,col).v != v) {
        vq_decref(vwCol(v,col).v);
        vwCol(v,col).v = v;
        vwCol(v,col).x.y.i = col;
    }
}

static void MutVecSetter (void *p, int row, int col, const vqCell *cp) {
    vqView v = p;
    int fill, miss;
    vqVec *vecp = (vqVec*) vwMore(v) + 3 * col;
    InitMutVec(v, col);
    if (row >= vwRows(v))
        vwRows(v) = row + 1;
    if (RangeLocate(vecp[0], row, &fill) & 1) {
        RangeFlip(vecp, row, 1);
        miss = RangeLocate(vecp[0], row, &row) & 1;
        assert(!miss);
        RangeInsert(vecp+1, row, 1, 0);
    } else
        row = fill;
    miss = RangeLocate(vecp[1], row, &fill) & 1;
    if (cp != 0) { /* set */
        if (miss) { /* add new value */
            RangeFlip(vecp+1, row, 1);
            RangeLocate(vecp[1], row, &fill);
            VecInsert(vecp+2, fill, 1);
        }
        vDisp(vecp[2])->setter(vecp[2], fill, col, cp);
    } else { /* unset */
        if (!miss) { /* remove existing value */
            vDisp(vecp[2])->setter(vecp[2], fill, col, 0);
            VecDelete(vecp+2, fill, 1);
            RangeFlip(vecp+1, row, 1);
        }
    }
}

static void MutVecReplacer (vqView t, int offset, int delrows, vqView data) {
    int c, r, cols = vwRows(vwMeta(t)), insrows = vwRows(data);
    assert(offset >= 0 && delrows >= 0 && offset + delrows <= vwRows(t));
    if (vInsv(t) == 0) {
        vInsv(t) = vq_incref(new_datavec(VQ_int, 2));
        vDelv(t) = vq_incref(new_datavec(VQ_int, 2));
        vSize(vInsv(t)) = 0;
        vSize(vDelv(t)) = 0;
    }
    vwRows(t) += insrows;
    for (c = 0; c < cols; ++c) {
        vqType coltype = vq_getInt(vwMeta(t), c, 1, VQ_nil) & VQ_TYPEMASK;
        vqVec *vecp = (vqVec*) vwMore(t) + 3 * c;
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
            if (c < vwRows(vwMeta(data)))
                for (r = 0; r < insrows; ++r) {
                    vqCell cell = vwCol(data,c);
                    if (getcell(r, &cell) == coltype)
                        MutVecSetter(t, offset + r, c, &cell);
                }
        }
    }
    vwRows(t) -= delrows;
    if (delrows > 0) {
        int pos, len = RangeSpan(vInsv(t), offset, delrows, &pos, 1);
        if (len > 0)
            RangeInsert(&vDelv(t), pos, len, 1);
        RangeDelete(&vInsv(t), offset, delrows);
    }
    if (insrows > 0)
        RangeInsert(&vInsv(t), offset, insrows, 1);
}

static void MutVecCleaner (void *p) {
    vqView v = p;
    vqVec *data = (vqVec*) vwMore(v);
    int i = 3 * vwRows(vwMeta(v));
    while (--i >= 0)
        vq_decref(data[i]);
    vq_decref(vDelv(v));
    vq_decref(vInsv(v));
    vq_decref(vOrig(v));
    vq_decref(vwMeta(v));
    vq_decref(vPerm(v));
    /* ObjRelease(vOref(v)); FIXME: need to release object somehow */
}

static vqDispatch muvtab = {
    "mutable", sizeof(struct vqView_s), sizeof(void*),
        MutVecCleaner, MutVecGetter, MutVecSetter, MutVecReplacer
};

int IsMutable (vqView t) {
    return vDisp(t) == &muvtab;
}

vqView MutWrapVop (vqView t) {
    vqView meta = vwMeta(t);
    int i, cols = vwRows(meta), ptrbytes = 3 * vwRows(meta) * sizeof(vqVec);
    vqView w = IndirectView(&muvtab, meta, vwRows(t),
                                ptrbytes + sizeof(struct MutVec_s));
    vwAuxP(w) = (char*) vwMore(w) + ptrbytes;
    vOrig(w) = vq_incref(t);
    /* vOref(w) = ObjRetain(o); FIXME: need to hold on to object somehow */
    /* override to use original columns until a set or replace is done */
    for (i = 0; i < cols; ++i) {
        vwCol(w,i) = vwCol(t,i);
        vq_incref(vwCol(w,i).v);
    }
    return w;
}

static int m_ismutable (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushboolean(L, IsMutable(A[0].v));
    return 1;
}

static int m_mutwrap (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return push_view(MutWrapVop(A[0].v));
}

static const struct luaL_reg lvqlib_mutable[] = {
    {"ismutable", m_ismutable},
    {"mutwrap", m_mutwrap},
    {0, 0},
};

#undef vInsv
#undef vDelv
#undef vOrig
#undef vPerm
