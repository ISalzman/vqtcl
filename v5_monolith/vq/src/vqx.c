/* Vlerq standalone exe with Xcode */

#include <stdio.h>

#include "vq4c.h"

int main (int argc, const char * argv[]) {
    vq_Item args[20];
    vq_Pool mypool = vq_addpool();
    
    vq_op(vq_lookup("all"), args);
    printf(" all %p\n", (void*) args[0].o.a.m);
    args[0].o.a.m = 0;
    args[1].o.a.i = 0;
    vq_op(vq_lookup("new"), args);
    printf(" new %p\n", (void*) args[0].o.a.m);
    vq_op(vq_lookup("meta"), args);
    printf("meta %p\n", (void*) args[0].o.a.m);
    vq_op(vq_lookup("meta"), args);
    printf(" m^2 %p\n", (void*) args[0].o.a.m);
    vq_op(vq_lookup("meta"), args);
    printf(" m^3 %p\n", (void*) args[0].o.a.m);
    printf("size %d\n", vq_size(args[0].o.a.m));

    vq_losepool(mypool);
    return 0;
}
