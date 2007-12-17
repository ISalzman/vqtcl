/*  Small Vlerq executable with datafile reading support.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

/* disable almost all optional code */

#define VQ_VBUFFER_H    0
#define VQ_VMUTABLE_H   0
#define VQ_VOPDEF_H     0
#define VQ_VRANGES_H    0
#define VQ_VSAVE_H      0

#include "vreader.c"
#include "vload.c"
#include "vlerq.c"

#include <stdio.h>

int main (int argc, char **argv) {
    vq_View v;
    int r;
    
    if (argc > 1) {
        v = OpenVop(argv[1]);
        if (v != NULL) {
            v = vq_meta(Vq_getView(v, 0, 0, 0));
            
            for (r = 0; r < vq_size(v); ++r)
                puts(Vq_getString(v, r, 0, ""));
        }
    }
    
    return 0;
}
