/*  Support for ranges and missing values.
    $Id: nulls.c 2053 2008-01-18 15:05:21Z jcw $
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static int *FixVec (vqColumn *vecp) {
    if (*vecp == 0) {
        *vecp = (vqColumn) NewPosVec(4);
        vSize(*vecp) = 0;
    }
    return (int*) *vecp;
}

void *RangeFlip (vqColumn *vecp, int off, int count) {
    int pos, end, *iptr = FixVec(vecp);
    if (count > 0) {
        end = vSize(*vecp);
        for (pos = 0; pos < end; ++pos, ++iptr)
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

int RangeLocate (vqColumn v, int off, int *offp) {
    int i = 0, last = 0, fill = 0;
    const int *ivec = (const int*) v;
    if (v != 0)
        while (i < vSize(v) && off >= ivec[i]) {
            last = ivec[i];
            if (i & 1)
                fill += last - ivec[i-1];
            ++i;
        }
    if (--i & 1)
        fill = last - fill;
    *offp = fill + off - last;
    assert(*offp >= 0);
    return i;
}

void RangeInsert (vqColumn *vecp, int off, int count, int mode) {
    int x, pos, miss, *ivec = FixVec(vecp);
    vqColumn v = (vqColumn) ivec;
    pos = RangeLocate(v, off, &x);
    miss = pos & 1;
    assert(count > 0);
    while (++pos < vSize(v))
        ivec[pos] += count;
    if (mode == miss)
        RangeFlip(vecp, off, count);
} 

void RangeDelete (vqColumn *vecp, int off, int count) {
    int pos, pos2, *ivec = FixVec(vecp);
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

static int r_flip (vqEnv env) {
    vqSlot *cp;
    vqView v = check_view(env, 1);
    int i = check_pos(env, 2), j = luaL_checkinteger(env, 3);
    cp = &v->col[0];
    RangeFlip(&cp->c, i, j);
    if (vInfo(v->col[0].c).owner.col == 0) /* TODO: awful cleanup hack */
        own_datavec(v, 0);
    vSize(v) = vSize(cp->c);
    lua_pop(env, 2);
    return 1;
}
static int r_locate (vqEnv env) {
    vqSlot *cp;
    vqView v = check_view(env, 1);
    int i = check_pos(env, 2), off;
    cp = &v->col[0];
    push_pos(env, RangeLocate(cp->c, i, &off));
    push_pos(env, off);
    return 2;
}
static int r_insert (vqEnv env) {
    vqSlot *cp;
    vqView v = check_view(env, 1);
    int i = check_pos(env, 2), j = luaL_checkinteger(env, 3),
        k = luaL_checkinteger(env, 4);
    cp = &v->col[0];
    RangeInsert(&cp->c, i, j, k);
    if (vInfo(v->col[0].c).owner.col == 0) /* TODO: awful cleanup hack */
        own_datavec(v, 0);
    vSize(v) = vSize(cp->c);
    lua_pop(env, 3);
    return 1;
}
static int r_delete (vqEnv env) {
    vqSlot *cp;
    vqView v = check_view(env, 1);
    int i = check_pos(env, 2), j = luaL_checkinteger(env, 3);
    cp = &v->col[0];
    RangeDelete(&cp->c, i, j);
    if (vInfo(v->col[0].c).owner.col == 0) /* TODO: awful cleanup hack */
        own_datavec(v, 0);
    vSize(v) = vSize(cp->c);
    lua_pop(env, 2);
    return 1;
}

static const struct luaL_reg vqlib_nulls[] = {
    {"r_flip", r_flip},
    {"r_locate", r_locate},
    {"r_insert", r_insert},
    {"r_delete", r_delete},
    {0, 0},
};
