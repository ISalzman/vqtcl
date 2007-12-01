/*  Core definitions and code.
    This file is part of LuaVlerq.
    See lvq.h for full copyright notice.
    $Id$  */

#include <stdlib.h>

typedef struct vq_Item vq_Item, *vq_View;

struct vq_Item {
    vq_View a;
    int b;
};

static vq_View vq_meta (vq_View v) {
    return v;
}

static int vq_size (vq_View v) {
    return 321;
}

static vq_View vq_new (vq_View meta, int rows) {
    return malloc(1);
}
