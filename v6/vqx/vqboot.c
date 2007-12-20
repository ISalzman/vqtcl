/*  Small Vlerq executable with datafile reading support.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

/* disable almost all optional code */

#include "vutil.c"
#include "vreader.c"
#include "vload.c"
#include "vlerq.c"

#include <stdio.h>

int main (int argc, char **argv) {
    vq_View v, m;
    int r;
    
    if (argc > 1) {
        v = OpenVop(argv[1]);
        if (v != NULL) {
            m = vq_meta(vq_getView(v, 0, 0, 0));
            for (r = 0; r < vq_size(m); ++r)
                puts(vq_getString(m, r, 0, ""));
        }
        vq_release(v);
    }
    
    return 0;
}
