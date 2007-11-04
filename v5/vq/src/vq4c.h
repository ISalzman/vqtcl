/* Vlerq public C header */

#include <stdint.h>
#include <unistd.h>

#define VQ_VERSION "5.0"

#define VQ_TYPES "NILFDSBTO"

typedef enum { 
    VQ_nil, 
    VQ_int,
    VQ_long,
    VQ_float,
    VQ_double,
    VQ_string,
    VQ_bytes, 
    VQ_table,
    VQ_object
} vq_Type;

typedef union vq_Item_u *vq_Table;
typedef vq_Table *vq_Pool;

typedef union vq_Item_u {
    struct {
        union {
            int                    i;
            float                  f;
            const char            *s;
            const uint8_t         *b;
            void                  *p;
            union vq_Item_u       *m;
            void                 (*c)(void*);
            struct vq_Dispatch_s  *h;
            int                   *n;
        } a;
        union {
            int               i;
            void             *p;
            union vq_Item_u  *m;
        } b;
    }        o;
    int64_t  w;
    double   d;
    char     r[sizeof(void*)*2];    /* for byte order flips */
    int      q[sizeof(void*)/2];    /* for hash calculations */
} vq_Item;

/* memory pools */

vq_Pool (vq_addpool) (void);
void   (vq_losepool) (vq_Pool pool);
vq_Table   (vq_hold) (vq_Table t);
vq_Table  (vq_holdf) (void *p, void (*f)(void*));
vq_Table (vq_retain) (vq_Table t);
void    (vq_release) (vq_Table t);

/* core table functions */

vq_Table    (vq_new) (vq_Table t, int bytes);
vq_Table   (vq_meta) (vq_Table t);
int        (vq_size) (vq_Table t);
vq_Item     (vq_get) (vq_Table t, int row, int col, vq_Type type, vq_Item def);
void        (vq_set) (vq_Table t, int row, int col, vq_Type type, vq_Item val);

/* wrappers */

int            (Vq_getInt) (vq_Table t, int row, int col, int def);
const char *(Vq_getString) (vq_Table t, int row, int col, const char *def);
vq_Table     (Vq_getTable) (vq_Table t, int row, int col, vq_Table def);

void           (Vq_setInt) (vq_Table t, int row, int col, int val);
void        (Vq_setString) (vq_Table t, int row, int col, const char *val);
void         (Vq_setTable) (vq_Table t, int row, int col, vq_Table val);
