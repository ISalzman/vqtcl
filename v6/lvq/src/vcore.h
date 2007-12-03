/*  Vlerq core public C header.
    $Id$
    This file is part of Vlerq, see lvq.h for the full copyright notice.  */

#include <stdint.h>
#include <unistd.h>

#define VQ_VERSION "1.6"
#define VQ_RELEASE "1.6.0"
#define VQ_COPYRIGHT "Copyright (C) 1996-2007 Jean-Claude Wippler"

#define VQ_TYPES "NILFDSBVO"

typedef enum { 
    VQ_nil, 
    VQ_int,
    VQ_long,
    VQ_float,
    VQ_double,
    VQ_string,
    VQ_bytes, 
    VQ_view,
    VQ_object
} vq_Type;

typedef union vq_Item_u *vq_View;
typedef vq_View *vq_Pool;

typedef union vq_Item_u {
    struct {
        union {
            int                    i;
            float                  f;
            const char            *s;
            const uint8_t         *b;
            void                  *p;
            union vq_Item_u       *v;
            void                 (*c)(void*);
            struct vq_Dispatch_s  *h;
            int                   *n;
        } a;
        union {
            int               i;
            void             *p;
            union vq_Item_u  *v;
        } b;
    }        o;
    int64_t  w;
    double   d;
    char     r[sizeof(void*)*2];    /* for byte order flips */
    int      q[sizeof(void*)/2];    /* for hash calculations */
} vq_Item;

/* reference counts */

vq_View  (vq_retain) (vq_View t);
void    (vq_release) (vq_View t);

/* core view functions */

vq_View     (vq_new) (vq_View t, int rows);
vq_View    (vq_meta) (vq_View t);
int        (vq_size) (vq_View t);
int       (vq_empty) (vq_View t, int row, int col);
vq_Item     (vq_get) (vq_View t, int row, int col, vq_Type type, vq_Item def);
void        (vq_set) (vq_View t, int row, int col, vq_Type type, vq_Item val);
void    (vq_replace) (vq_View t, int start, int count, vq_View data);

/* wrappers */

int            (Vq_getInt) (vq_View t, int row, int col, int def);
const char *(Vq_getString) (vq_View t, int row, int col, const char *def);
vq_View       (Vq_getView) (vq_View t, int row, int col, vq_View def);

void         (Vq_setEmpty) (vq_View t, int row, int col);
void           (Vq_setInt) (vq_View t, int row, int col, int val);
void        (Vq_setString) (vq_View t, int row, int col, const char *val);
void          (Vq_setView) (vq_View t, int row, int col, vq_View val);
