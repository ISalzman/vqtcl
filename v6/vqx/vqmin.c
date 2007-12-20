/*  Minimal C-only Vlerq executable.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vlerq.c"
#include <stdio.h>

int main (void) {
    vq_View m, v;
    int r, rows, c, cols;
    
    m = vq_new(3, vq_meta(0));
    vq_setMetaRow (m, 0, "A", VQ_int, 0);
    vq_setMetaRow (m, 1, "B", VQ_int, 0);
    vq_setMetaRow (m, 2, "C", VQ_int, 0);
    
    v = vq_new(4, m);
    rows = vq_size(v);
    cols = vq_size(vq_meta(v));

    for (r = 0; r < rows; ++r)
        for (c = 0; c < cols; ++c)
            vq_setInt(v, r, c, 1+r+c);
    
    for (r = 0; r < rows; ++r) {
        printf(" %d:", r);
        for (c = 0; c < cols; ++c)
            printf(" %d", vq_getInt(v, r, c, -1));
        printf("\n");
    }
    
    vq_release(v);
    return 0;
}
