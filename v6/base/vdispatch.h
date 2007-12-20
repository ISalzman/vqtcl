/* vdispatch.h - generated code, do not edit */

#ifndef VQ_DISPATCH_H
#define VQ_DISPATCH_H 1

typedef struct {
    const char *name, *args;
    vq_Type (*proc)(vq_Cell*);
} CmdDispatch;

extern CmdDispatch f_vdispatch[];

#endif

/* end of generated code */
