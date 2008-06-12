/*  Support for mutable views.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

typedef struct MutVec_s {
    vqSlot insv, delv;
    vqView orig;
} *MutVec;
                    
#define vInsv(v)   ((MutVec) vwAuxP(v))->insv.c
#define vDelv(v)   ((MutVec) vwAuxP(v))->delv.c
#define vOrig(v)   ((MutVec) vwAuxP(v))->orig

static int RangeSpan (vqColumn v, int offset, int count, int *startp, int miss) {
    int rs, ps = RangeLocate(v, offset, &rs);
    int re, pe = RangeLocate(v, offset + count, &re);
    if ((ps ^ miss) & 1)
        rs = offset - rs;
    if ((pe ^ miss) & 1)
        re = offset + count - re;
    *startp = rs;
    return re - rs;
}

static int RangeExpand (vqColumn v, int off) {
    int i;
    const int *ivec = (const int*) v;
    for (i = 0; i < vSize(v) && ivec[i] <= off; i += 2)
        off += ivec[i+1] - ivec[i];
    return off;
}

static vqType MutVecGetter (int row, vqSlot *cp) {
    int col = cp->x.y.i, aux = row;
    vqView v = cp->v;
    vqSlot *vecp = (vqSlot*) vwMore(v) + 3 * col;
    if (vecp[0].c != 0 && RangeLocate(vecp[0].c, row, &aux) & 1) { /* transl. */
        if (vInsv(v) != 0 && (RangeLocate(vInsv(v), row, &row) & 1) == 0)
            return VQ_nil;
        if (vDelv(v) != 0)
            row = RangeExpand(vDelv(v), row);
        /* avoid t[col] dereference of vq_new-type views, which have no rows */
        if (row < vSize(vOrig(v))) {
            *cp = vOrig(v)->col[col];
            if (cp->v != 0)
                return getcell(row, cp);
        }
    } else { /* opaque */
        if ((vecp[1].c == 0 || (RangeLocate(vecp[1].c, aux, &aux) & 1) == 0)) {
            cp->c = vecp[2].c;
            if (cp->c != 0)
                return getcell(aux, cp);
        }
    }
    return VQ_nil;
}

static void InitMutVec (vqView v, int col) {
    int pos = vwCols(v) + 3 * col;
    vqSlot *vecp = v->col + pos;
    if (vecp[0].c == 0) {
        /* TODO: try to avoid allocating all vectors right away */
        vecp[0].c = new_datavec(VQ_int, 2);
        vecp[1].c = new_datavec(VQ_int, 2);
        vecp[2].c = new_datavec(ViewColType(v, col), 2);
        vSize(vecp[0].c) = 0;
        vSize(vecp[1].c) = 0;
        vSize(vecp[2].c) = 0;
        own_datavec(v, pos);
        own_datavec(v, pos+1);
        own_datavec(v, pos+2);
    }
    /* sneaky: make sure getter always goes through mutvec from now on */
    if (v->col[col].v != v) {
        /* TODO broken with new refcounts: vq_decref(v->col[col].v); */
        v->col[col].v = v;
        v->col[col].x.y.i = col;
    }
}

static void MutVecSetter (vqColumn p, int row, int col, const vqSlot *cp) {
    vqView v = p->asview;
    int fill, miss;
    vqSlot *vecp = (vqSlot*) vwMore(v) + 3 * col;
    InitMutVec(v, col);
    if (row >= vSize(v))
        vSize(v) = row + 1;
    if (RangeLocate(vecp[0].c, row, &fill) & 1) {
        RangeFlip(&vecp[0].c, row, 1);
        miss = RangeLocate(vecp[0].c, row, &row) & 1;
        assert(!miss);
        RangeInsert(&vecp[1].c, row, 1, 0);
    } else
        row = fill;
    miss = RangeLocate(vecp[1].c, row, &fill) & 1;
    if (cp != 0) { /* set */
        if (miss) { /* add new value */
            RangeFlip(&vecp[1].c, row, 1);
            RangeLocate(vecp[1].c, row, &fill);
            VecInsert(&vecp[2].c, fill, 1);
        }
        vInfo(vecp[2].c).dispatch->setter(vecp[2].c, fill, col, cp);
    } else { /* unset */
        if (!miss) { /* remove existing value */
            vInfo(vecp[2].c).dispatch->setter(vecp[2].c, fill, col, 0);
            VecDelete(&vecp[2].c, fill, 1);
            RangeFlip(&vecp[1].c, row, 1);
        }
    }
}

static void MutVecReplacer (vqView t, int offset, int delrows, vqView data) {
    int c, r, cols = vwCols(t), insrows = vSize(data);
    assert(offset >= 0 && delrows >= 0 && offset + delrows <= vSize(t));
    if (vInsv(t) == 0) {
        vInsv(t) = new_datavec(VQ_int, 2);
        vDelv(t) = new_datavec(VQ_int, 2);
        vSize(vInsv(t)) = 0;
        vSize(vDelv(t)) = 0;
        own_datavec(t, 4 * vwCols(t));
        own_datavec(t, 4 * vwCols(t) + 1);
    }
    vSize(t) += insrows;
    for (c = 0; c < cols; ++c) {
        vqType coltype = ViewColType(t, c);
        vqSlot *vecp = (vqSlot*) vwMore(t) + 3 * c;
        InitMutVec(t, c);
        if (delrows > 0) {
            int pos, len = RangeSpan(vecp[0].c, offset, delrows, &pos, 0);
            /* clear all entries to set up contiguous nil range */
            /* TODO: optimize and delete current values instead */
            for (r = 0; r < delrows; ++r)
                MutVecSetter((vqColumn) t, offset + r, c, 0);
            if (len > 0)
                RangeDelete(&vecp[1].c, pos, len);
            RangeDelete(&vecp[0].c, offset, delrows);
        }
        if (insrows > 0) {
            RangeInsert(&vecp[0].c, offset, insrows, 0); /* new transl. range */
            if (c < vwCols(data))
                for (r = 0; r < insrows; ++r) {
                    vqSlot cell = data->col[c];
                    if (getcell(r, &cell) == coltype)
                        MutVecSetter((vqColumn) t, offset + r, c, &cell);
                }
        }
    }
    vSize(t) -= delrows;
    if (delrows > 0) {
        int pos, len = RangeSpan(vInsv(t), offset, delrows, &pos, 1);
        if (len > 0)
            RangeInsert(&vDelv(t), pos, len, 1);
        RangeDelete(&vInsv(t), offset, delrows);
    }
    if (insrows > 0)
        RangeInsert(&vInsv(t), offset, insrows, 1);
}

static void MutVecCleaner (vqColumn cp) {
    vqView v = cp->asview;
    vqSlot *data = (vqSlot*) vwMore(v);
    int i = 3 * vwCols(v);
    while (--i >= 0)
        release_vec(data[i].c);
    release_vec(vDelv(v));
    release_vec(vInsv(v));
    vq_decref(vOrig(v));
    vq_decref(vwMeta(v));
    /* ObjRelease(vOref(v)); FIXME: need to release object somehow */
}

static vqDispatch muvtab = {
    "mutable", sizeof(struct vqView_s), 0,
        MutVecCleaner, MutVecGetter, MutVecSetter, MutVecReplacer
};

int IsMutable (vqView t) {
    return vInfo(t).dispatch == &muvtab;
}

vqView MutWrapVop (vqView t) {
    vqView meta = vwMeta(t);
    int i, cols = vSize(meta), ptrbytes = 3 * vSize(meta) * sizeof(vqSlot);
    vqView w = IndirectView(&muvtab, meta, vSize(t),
                                ptrbytes + sizeof(struct MutVec_s));
    vwAuxP(w) = (char*) vwMore(w) + ptrbytes;
    vOrig(w) = vq_incref(t);
    /* override to use original columns until a set or replace is done */
    for (i = 0; i < cols; ++i) {
        w->col[i] = t->col[i];
        incColRef(w->col[i].p);
    }
    return w;
}

static int m_ismutable (vqEnv env) {
    lua_pushboolean(env, IsMutable(check_view(env, 1)));
    return 1;
}

static int m_mutwrap (vqEnv env) {
    return push_view(MutWrapVop(check_view(env, 1)));
}

static const struct luaL_reg vqlib_mutable[] = {
    {"ismutable", m_ismutable},
    {"mutwrap", m_mutwrap},
    {0, 0},
};

#undef vInsv
#undef vDelv
#undef vOrig
