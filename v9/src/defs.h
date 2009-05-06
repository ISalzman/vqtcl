#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define vUNUSED(x)  ((void)(x)) /* to avoid warnings */

typedef struct {
    void *aux;
    vView empty;
} vEnv;

typedef struct {
    const char *name;
    char type;
    char itemsize;
    char vslots;
    char vbytes;
    void (*cleaner) (vColPtr);
    vType (*getter) (int,vAny*);
    void (*setter) (vColPtr,int,int,const vAny*);
} vDispatch;

#define SEQ_HEADER \
    const vDispatch *dispatch; \
    int refs; \
    int size; \
    /* TODO vAny parent; */

struct vSequence_s {
    SEQ_HEADER
};

struct vColPtr_s {
    SEQ_HEADER
    union {
        int i [1];
        vLong l [1];
        float f [1];
        double d [1];
        void *p [1];
        vAny a [1];
    } items;
};

#define vMETA(vw) (vw->mvarc.v)
#define vROWS(vw) (vw->mvarc.pair.two.j)

#define vENV(vw) ((vEnv*) vMETA(vMETA(vw))->columns[3].p)
#define vCOLTYPE(vw,col) ((vType) (vw)->columns[col].c->dispatch->type)
#define vISMETA(vw) (vMETA(vw) == vMETA(vMETA(vw)))

struct vView_s {
    SEQ_HEADER
    vAny mvarc; /* meta-view pointer and row count */
    vAny columns [1];
};

/* datacol.c */
vColPtr (newDataColumn) (vEnv *env, vType type, int size);

/* utils.c */
void (releaseColumn) (vColPtr cptr);
vColPtr (incRef) (vColPtr cptr);
void (decRef) (vColPtr cptr);
vType (getType) (vView vw, int col);
vType (getItem) (vView vw, int row, int col, vAny *vec);
int (getIntItem) (vView vw, int row, int col);
void (setColumn) (vView vw, int col, vColPtr cptr);
void (setItem) (vView vw, int row, int col, const vAny *val);
void (setIntItem) (vView vw, int row, int col, int val);
void (setStringItem) (vView vw, int row, int col, const char *val);
void (setViewItem) (vView vw, int row, int col, vView val);
vView (newView) (vView meta, int rows);
vEnv *(initEnv) (void *aux);
