/*  Support for ranges and missing values.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

void *VecInsert (vqVec *vecp, int off, int cnt) {
    vqVec v = *vecp, v2;
    int unit = vDisp(v)->unit, limit = vExtra(v);
    char *cvec = (char*) v, *cvec2;
    assert(cnt > 0);
    assert(unit > 0);
    assert(vRefs(v) == 1);
    assert(off <= vSize(v));
    if (vSize(v) + cnt <= limit) {
        memmove(cvec+(off+cnt)*unit, cvec+off*unit, (vSize(v)-off)*unit);
        memset(cvec+off*unit, 0, cnt*unit);
    } else {
        limit += limit/2;
        if (limit < vSize(v) + cnt)
            limit = vSize(v) + cnt;
        v2 = vq_incref(alloc_vec(vDisp(v), limit * unit));
        cvec2 = (char*) v2;
        vSize(v2) = vSize(v);
        vExtra(v2) = limit;
        memcpy(v2, v, off * unit);
        memcpy(cvec2+(off+cnt)*unit, cvec+off*unit, (vSize(v)-off)*unit);
        vSize(v) = 0; /* prevent cleanup of copied elements */
        vq_decref(v);
        *vecp = v = v2;
    }
    vSize(v) += cnt;
    return v;
}

void *VecDelete (vqVec *vecp, int off, int cnt) {
    /* TODO: shrink when less than half full and > 10 elements */
    vqVec v = *vecp;
    int unit = vDisp(v)->unit;
    char *cvec = (char*) v;
    assert(cnt > 0);
    assert(unit > 0);
    assert(vRefs(v) == 1);
    assert(off + cnt <= vSize(v));
    memmove(cvec+off*unit, cvec+(off+cnt)*unit, (vSize(v)-(off+cnt))*unit);
    vSize(v) -= cnt;
    return v;
} 

void *RangeFlip (vqVec *vecp, int off, int count) {
    int pos, end, *iptr;
    if (*vecp == 0) {
        *vecp = vq_incref(new_datavec(VQ_int, 4));
        vSize(*vecp) = 0;
    }
    if (count > 0) {
        end = vSize(*vecp);
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

int RangeLocate (vqVec v, int off, int *offp) {
    int i, last = 0, fill = 0;
    const int *ivec = (const int*) v;
    for (i = 0; i < vSize(v) && off >= ivec[i]; ++i) {
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

void RangeInsert (vqVec *vecp, int off, int count, int mode) {
    vqVec v = *vecp;
    int x, pos = RangeLocate(v, off, &x), miss = pos & 1, *ivec = (int*) v;
    assert(count > 0);
    while (++pos < vSize(v))
        ivec[pos] += count;
    if (mode == miss)
        RangeFlip(vecp, off, count);
} 

void RangeDelete (vqVec *vecp, int off, int count) {
    int pos, pos2;
    int *ivec = (int*) *vecp;
    assert(count > 0);
    /* very tricky code because more than one range may have to be deleted */
    for (pos = 0; pos < vSize(*vecp); ++pos)
        if (ivec[pos] >= off) {
            for (pos2 = pos; pos2 < vSize(*vecp); ++pos2)
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
            while (pos < vSize(*vecp))
                ivec[pos++] -= count;
            break;
        }
}

static int r_flip (lua_State *L) {
    vqCell *cp;
    LVQ_ARGS(L,A,"VII");
    cp = &vwCol(A[0].v,0);
    RangeFlip((vqVec*) &cp->p, A[1].i, A[2].i);
    vwRows(A[0].v) = vSize(cp->p);
    lua_pop(L, 2);
    return 1;
}
static int r_locate (lua_State *L) {
    vqCell *cp;
    int off;
    LVQ_ARGS(L,A,"VI");
    cp = &vwCol(A[0].v,0);
    lua_pushinteger(L, RangeLocate(cp->p, A[1].i, &off));
    lua_pushinteger(L, off);
    return 2;
}
static int r_insert (lua_State *L) {
    vqCell *cp;
    LVQ_ARGS(L,A,"VIII");
    cp = &vwCol(A[0].v,0);
    RangeInsert((vqVec*) &cp->p, A[1].i, A[2].i, A[3].i);
    vwRows(A[0].v) = vSize(cp->p);
    lua_pop(L, 3);
    return 1;
}
static int r_delete (lua_State *L) {
    vqCell *cp;
    LVQ_ARGS(L,A,"VII");
    cp = &vwCol(A[0].v,0);
    RangeDelete((vqVec*) &cp->p, A[1].i, A[2].i);
    vwRows(A[0].v) = vSize(cp->p);
    lua_pop(L, 2);
    return 1;
}

static const struct luaL_reg lvqlib_ranges[] = {
    {"r_flip", r_flip},
    {"r_locate", r_locate},
    {"r_insert", r_insert},
    {"r_delete", r_delete},
    {0, 0},
};
