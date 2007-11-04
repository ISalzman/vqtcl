/* Load views from a Metakit-format memory map */

#include "defs.h"

#include <stdlib.h>
#include <string.h>

#pragma mark - RANGE OPERATIONS -

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

void *RangeFlip (Vector *vecp, int off, int count) {
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
int RangeLocate (Vector v, int off, int *offp) {
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
void RangeInsert (Vector *vecp, int off, int count, int mode) {
    Vector v = *vecp;
    int x, pos = RangeLocate(v, off, &x), miss = pos & 1, *ivec = (int*) v;
    assert(count > 0);
    while (++pos < vCount(v))
        ivec[pos] += count;
    if (mode == miss)
        RangeFlip(vecp, off, count);
} 
void RangeDelete (Vector *vecp, int off, int count) {
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
