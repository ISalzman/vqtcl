/*  Additional view operators.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#if VQ_OPDEF_H

static vq_Type IotaGetter (int row, vq_Item *item) {
    item->o.a.i = row + vOffs(item->o.a.v);
    return VQ_int;
}
static Dispatch iotatab = {
    "iota", 3, 0, 0, IndirectCleaner, IotaGetter
};

vq_View IotaView (int rows, const char *name, int base) {
    vq_View v, meta = vq_new(1, vq_meta(0));
    Vq_setMetaRow(meta, 0, name, VQ_int, NULL);
    v = IndirectView(meta, &iotatab, rows, 0);
    vOffs(v) = base;
    return v;
}

vq_View PassView (vq_View v) {
    vq_View t = vq_new(0, vMeta(v));
    int c, cols = vCount(vMeta(v));
    vCount(t) = vCount(v);
    for (c = 0; c < cols; ++c) {
        t[c] = v[c];
        vq_retain(t[c].o.a.v);
    }
    return t;
}

#endif
