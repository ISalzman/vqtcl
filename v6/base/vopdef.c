/*  Additional view operators.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#if VQ_OPDEF_H

static vq_Type StepGetter (int row, vq_Item *item) {
    Vector v = vData(item->o.a.v);
    item->o.a.i = v[0].o.a.i + v[0].o.b.i * (row / v[1].o.a.i);
    return VQ_int;
}
static Dispatch steptab = {
    "step", 3, 0, 0, IndirectCleaner, StepGetter
};

vq_View StepView (int rows, int start, int step, int rate, const char *name) {
    vq_View v, meta = vq_new(1, vq_meta(0));
    Vq_setMetaRow(meta, 0, name != NULL ? name : "", VQ_int, NULL);
    v = IndirectView(meta, &steptab, rows, 2 * sizeof(vq_Item));
    vData(v)[0].o.a.i = start;
    vData(v)[0].o.b.i = step;
    vData(v)[1].o.a.i = rate > 0 ? rate : 1;
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
