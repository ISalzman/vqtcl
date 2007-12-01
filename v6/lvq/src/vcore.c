#include <stdlib.h>

typedef struct vq_Item *vq_View;

struct vq_Item {
    int a, b;
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
