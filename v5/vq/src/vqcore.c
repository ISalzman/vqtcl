/* Vlerq core */

#include "vqdefs.h"

#include <stdlib.h>
#include <string.h>

#define POOL_CHUNK 64

/* TODO: make globals thread-safe */
static vq_Pool currentPool;

#pragma mark - MEMORY MANAGEMENT -

Vector AllocVector (Dispatch *vtab, int bytes) {
    Vector result;
    bytes += vtab->prefix * sizeof(vq_Item);
    result = (Vector) calloc(1, bytes) + vtab->prefix;
    vType(result) = vtab;
    return result;
}

void FreeVector (Vector v) {
    free(v - vType(v)->prefix);
}

#pragma mark - MEMORY POOLS -

static vq_Pool NewPoolChunk (vq_Pool prev) {
    vq_Pool pool = malloc(POOL_CHUNK * sizeof(void*));
    pool[0] = (void*) prev;
    pool[1] = (void*) (pool + 2);
    return pool;
}
static void RestorePool (void *p) {
    currentPool = p;
}
vq_Pool vq_addpool (void) {
    vq_Pool prevPool = currentPool;
    currentPool = NewPoolChunk(0);
    vq_holdf(prevPool, &RestorePool);
    return currentPool;
}
void vq_losepool (vq_Pool pool) {
    /* TODO: deal with pool != currentPool */
    do {
        vq_Pool next = (void*) pool[0];
        void **rover = (void*) pool[1];
        while (*--rover >= (void*) (pool + 2))
            vq_release(*rover);
        free(pool);
        pool = next;
    } while (pool != 0);
}
Vector vq_hold (Vector v) {
    void **rover;
    if ((void*) currentPool[1] >= (void*) (currentPool + POOL_CHUNK))
        currentPool = NewPoolChunk(currentPool);
    rover = (void*) currentPool[1];
    *rover++ = vq_retain(v);
    currentPool[1] = (void*) rover;
    return v;
}
static void HoldCleaner (Vector v) {
    v[0].o.a.c(v[0].o.b.p);
    FreeVector(v);
}
Vector vq_holdf (void *p, void (*f)(void*)) {
    static Dispatch vtab = { "hold", 1, 0, 0, HoldCleaner };
    Vector v = vq_hold(AllocVector(&vtab, sizeof *v));
    v[0].o.a.c = f;
    v[0].o.b.p = p;
    return v;
}
/* the above code doesn't need to know anything about reference counting */
Vector vq_retain (Vector v) {
    if (v != 0)
        ++vRefs(v); /* TODO: check for overflow */
    return v;
}
void vq_release (Vector v) {
    if (v != 0 && --vRefs(v) <= 0) {
        /*assert(vRefs(v) == 0);*/
        if (vType(v)->cleaner != 0)
            vType(v)->cleaner(v);
        else
            FreeVector(v); /* TODO: when is this case needed? */
    }
}

#pragma mark - CORE TABLE FUNCTIONS -

vq_Type GetItem (int row, vq_Item *item) {
    Vector v = item->o.a.m;
    assert(vType(v)->getter != 0);
    return vType(v)->getter(row, item);
}

vq_Table vq_meta (vq_Table t) {
    if (t == 0)
        t = EmptyMetaTable();
    return vMeta(t);
}
int vq_size (vq_Table t) {
    return t != 0 ? vCount(t) : 0;
}

vq_Item vq_get (vq_Table t, int row, int column, vq_Type type, vq_Item def) {
    vq_Item item;
    if (row < 0 || row >= vCount(t) || column < 0 || column >= vCount(vMeta(t)))
        return def;
    item = t[column];
    return GetItem(row, &item) != VQ_nil ? item : def;
}

#pragma mark - UTILITY WRAPPERS -

int Vq_getInt (vq_Table t, int row, int col, int def) {
    vq_Item item;
    item.o.a.i = def;
    item = vq_get(t, row, col, VQ_int, item);
    return item.o.a.i;
}

const char *Vq_getString (vq_Table t, int row, int col, const char *def) {
    vq_Item item;
    item.o.a.s = def;
    item = vq_get(t, row, col, VQ_string, item);
    return item.o.a.s;
}

vq_Table Vq_getTable (vq_Table t, int row, int col, vq_Table def) {
    vq_Item item;
    item.o.a.m = def;
    item = vq_get(t, row, col, VQ_table, item);
    return item.o.a.m;
}

#pragma mark - OPERATOR WRAPPERS -

static vq_Type AtCmd_TIIO (vq_Item a[]) {
    vq_Table t = a[0].o.a.m;
    vq_Type type = Vq_getInt(vMeta(t), a[2].o.a.i, 1, VQ_nil) & VQ_TYPEMASK;
    if (type != VQ_nil && ObjToItem(type, a+3)) {
        *a = vq_get(t, a[1].o.a.i, a[2].o.a.i, type, a[3]);
        return type;
    }
    *a = a[3];
    return VQ_object;
}
static vq_Type Debug_OX (vq_Item a[]) {
    a->o.a.p = DebugCode(a[0].o.a.p, a[1].o.b.i, a[1].o.a.p);
    return VQ_object;
}
static vq_Type MetaCmd_T (vq_Item a[]) {
    a->o.a.m = vq_meta(a[0].o.a.m);
    return VQ_table;
}
static vq_Type NewCmd_T (vq_Item a[]) {
    a->o.a.m = vq_new(a[0].o.a.m);
    return VQ_table;
}
static vq_Type SizeCmd_T (vq_Item a[]) {
    vq_Table t = vq_new(0);
    vCount(t) = vq_size(a[0].o.a.m);
    a->o.a.m = t;
    return VQ_table;
}

#pragma mark - GENERIC OPERATORS -

CmdDispatch f_commands[] = {
    { "at",      "O:TIIO", AtCmd_TIIO      },
    { "debug",   "O:OX",   Debug_OX         },
    { "meta",    "T:T",    MetaCmd_T       },
    { "new",     "T:T",    NewCmd_T        },
    { "size",    "T:T",    SizeCmd_T       },
    { 0, 0, 0 }
};  
