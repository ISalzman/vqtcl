/*  Additional view operators.
    $Id$
    This file is part of Vlerq, see lvq.h for the full copyright notice.  */

#include "vdefs.h"

#if VQ_MOD_OPDEF

#pragma mark - IOTA VIEW -

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

static vq_Type IotaGetter (int row, vq_Item *item) {
    item->o.a.i = row;
    return VQ_int;
}
static Dispatch iotatab = {
    "iota", 3, 0, 0, IndirectCleaner, IotaGetter
};
vq_View IotaView (int rows, const char *name) {
    vq_View meta = vq_new(vq_meta(0), 1);
    Vq_setString(meta, 0, 0, name);
    Vq_setInt(meta, 0, 1, VQ_int);
    Vq_setView(meta, 0, 2, EmptyMetaView());
    return IndirectView(meta, &iotatab, rows, 0);
}

#pragma mark - PASS AS NEW VIEW -

vq_View PassView (vq_View v) {
    vq_View t;
    int c, cols;
    t = vq_new(vMeta(v), 0);
    cols = vCount(vMeta(v));
    vCount(t) = vCount(v);
    for (c = 0; c < cols; ++c) {
        t[c] = v[c];
        vq_retain(t[c].o.a.v);
    }
    return t;
}

#endif
