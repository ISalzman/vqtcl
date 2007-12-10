/*  Minimal C-only Vlerq executable.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice.  */

/* disable all optional code */

#define VQ_VBUFFER_H    0
#define VQ_VLOAD_H      0
#define VQ_VMUTABLE_H   0
#define VQ_VOPDEF_H     0
#define VQ_VRANGES_H    0
#define VQ_VREADER_H    0
#define VQ_VSAVE_H      0

#include "vlerq.c"

#include <stdio.h>

int main (int argc, char **argv) {
    vq_View m, v;
    int r, rows, c, cols;
    
    m = vq_new(6, vq_meta(0));

    Vq_setMetaRow (m, 0, "A", VQ_int, 0);
    Vq_setMetaRow (m, 1, "B", VQ_int, 0);
    Vq_setMetaRow (m, 2, "C", VQ_int, 0);
    Vq_setMetaRow (m, 3, "D", VQ_int, 0);
    Vq_setMetaRow (m, 4, "E", VQ_int, 0);
    Vq_setMetaRow (m, 5, "F", VQ_int, 0);
    
    v = vq_new(4, m);

    rows = vq_size(v);
    cols = vq_size(vq_meta(v));

    for (r = 0; r < rows; ++r)
        for (c = 0; c < cols; ++c)
            Vq_setInt(v, r, c, 1+r+c);
    
    for (r = 0; r < rows; ++r) {
        printf(" %d:", r);
        for (c = 0; c < cols; ++c)
            printf(" %d", Vq_getInt(v, r, c, -1));
        printf("\n");
    }
    
    return 0;
}
