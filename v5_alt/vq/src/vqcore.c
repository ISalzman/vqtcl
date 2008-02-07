/* Vlerq core */

#include "vqdefs.h"

#include <stdlib.h>
#include <string.h>

/* TODO: make globals thread-safe */
static Buffer_p  currentPool;
static vq_Table  opTable;

/* forward */
static int (NumOperators) (void);

#pragma mark - MEMORY POOLS -

static void RestorePool (void *p) {
    currentPool = p;
}
/* TODO: change API to pass a stack-based buffer as arg */
vq_Pool vq_addpool (void) {
    Buffer_p prevPool = currentPool;
    currentPool = malloc(sizeof *currentPool);
    InitBuffer(currentPool);
    /* first added entry is the one last executed, restores previous pool */
    vq_holdf(prevPool, &RestorePool);
    return currentPool;
}
void vq_losepool (vq_Pool pool) {
    if (pool == currentPool) {
        int cnt;
        char *ptr = 0;
        while (NextBuffer(pool, &ptr, &cnt)) {
            Vector *v = (Vector*) ptr;
            int i = cnt / sizeof(Vector);
            while (--i >= 0)
                vq_release(v[i]);
        }
        ReleaseBuffer(pool, 0);
        free(pool);
    }
    /* TODO: deal with pool != currentPool */
}
Vector vq_hold (Vector v) {
    ADD_PTR_TO_BUF(*currentPool, v);
    return vq_retain(v);
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

int vq_empty (vq_Table t, int row, int column) {
    vq_Item item;
    if (column < 0 || column >= vCount(vMeta(t)))
        return 1;
    item = t[column];
    return GetItem(row, &item) == VQ_nil;
}
vq_Item vq_get (vq_Table t, int row, int column, vq_Type type, vq_Item def) {
    vq_Item item;
    if (row < 0 || row >= vCount(t) || column < 0 || column >= vCount(vMeta(t)))
        return def;
    item = t[column];
    return GetItem(row, &item) != VQ_nil ? item : def;
}
void vq_set (vq_Table t, int row, int column, vq_Type type, vq_Item val) {
    Vector v;
    assert(IsMutable(t));
    assert(0 <= column && column < vCount(vMeta(t)));
    v = t[column].o.a.m;
    MutVecSet(v, row, t[column].o.b.i, type != VQ_nil ? &val : 0);
}
void vq_replace (vq_Table t, int start, int count, vq_Table data) {
    assert(IsMutable(t));
    assert(start >= 0 && count >= 0 && start + count <= vCount(t));
    MutVecReplace(t, start, count, data);
}

int vq_lookup (const char *opname) {
    if (opTable == 0) {
        int i, n = NumOperators();
        vq_Table meta = WrapMutable(vq_new(vq_meta(0)));
        Vq_setString(meta, 0, 0, "name");
        Vq_setString(meta, 1, 0, "args");
        Vq_setInt(meta, 0, 1, VQ_string);
        Vq_setInt(meta, 1, 1, VQ_string);
        opTable = vq_retain(WrapMutable(vq_new(meta)));
        for (i = 0; i < n; ++i) {
            Vq_setString(opTable, i, 0, f_commands[i].name);
            Vq_setString(opTable, i, 1, f_commands[i].args);
            /* TODO: add proc column? */
        }
    }
    if (opname != 0) {
        int i, n = vCount(opTable);
        for (i = 0; i < n; ++i)
            if (strcmp(opname, Vq_getString(opTable, i, 0, "")) == 0)
                return i;
    }
    return -1;
}

#pragma mark - OPERATOR WRAPPERS -

static vq_Type AllCmd_N (vq_Item* a) {
    vq_lookup(0); /* force init of optable */
    a->o.a.m = opTable;
    return VQ_table;
}
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
static vq_Type EmptyCmd_TII (vq_Item a[]) {
    a->o.a.i = vq_empty(a[0].o.a.m, a[1].o.a.i, a[2].o.a.i);
    return VQ_int;
}
static vq_Type IotaCmd_TS (vq_Item a[]) {
    a->o.a.m = IotaTable(vq_size(a[0].o.a.m), a[1].o.a.s);
    return VQ_table;
}
/*
static vq_Type MdefCmd_O (vq_Item a[]) {
    a->o.a.m = ObjAsMetaTable(a[0].o.a.p);
    return VQ_table;
}
*/
static vq_Type MetaCmd_T (vq_Item a[]) {
    a->o.a.m = vq_meta(a[0].o.a.m);
    return VQ_table;
}
static vq_Type NewCmd_T (vq_Item a[]) {
    a->o.a.m = vq_new(a[0].o.a.m);
    return VQ_table;
}
static vq_Type ReplaceCmd_MIIT (vq_Item a[]) {
    vq_Table t = ObjAsTable(a[0].o.b.p);
    vq_replace(t, a[1].o.a.i, a[2].o.a.i, a[3].o.a.m);
    UpdateVar(a[0]);
    return VQ_nil;
}
static vq_Type SetCmd_MIIO (vq_Item a[]) {
    vq_Table t = ObjAsTable(a[0].o.b.p);
    int row = a[1].o.a.i, column = a[2].o.a.i;
    vq_Type type = Vq_getInt(vMeta(t), column, 1, VQ_nil) & VQ_TYPEMASK;
    if (ObjToItem(type, a+3)) {
        if (row >= vCount(t))
            vCount(t) = row + 1;
        vq_set(t, row, column, type, a[3]);
        UpdateVar(a[0]);
    }
    return VQ_nil;
}
static vq_Type SizeCmd_T (vq_Item a[]) {
    vq_Table t = vq_new(0);
    vCount(t) = vq_size(a[0].o.a.m);
    a->o.a.m = t;
    return VQ_table;
}
static vq_Type UnsetCmd_MII (vq_Item a[]) {
    vq_Table t = ObjAsTable(a[0].o.b.p);
    int row = a[1].o.a.i, column = a[2].o.a.i;
    if (row >= vCount(t))
        vCount(t) = row + 1;
    vq_set(t, row, column, VQ_nil, a[0]);
    UpdateVar(a[0]);
    return VQ_nil;
}

#pragma mark - GENERIC OPERATORS -

CmdDispatch f_commands[] = {
    { "all",     "T:",     AllCmd_N        },
    { "at",      "O:TIIO", AtCmd_TIIO      },
 /* { "data",    "T:TX",   DataCmd_TX      }, */
    { "debug",   "O:OX",   Debug_OX         },
    { "empty",   "I:TII",  EmptyCmd_TII    },
    { "iota",    "I:TS",   IotaCmd_TS      },
 /* { "mdef",    "T:O",    MdefCmd_O       }, */
    { "meta",    "T:T",    MetaCmd_T       },
    { "new",     "T:T",    NewCmd_T        },
    { "replace", "V:MIIT", ReplaceCmd_MIIT },
    { "set",     "V:MIIO", SetCmd_MIIO     },
    { "size",    "T:T",    SizeCmd_T       },
    { "unset",   "V:MII",  UnsetCmd_MII    },
    { 0, 0, 0 }
};  
static int NumOperators (void) {
    return sizeof f_commands / sizeof *f_commands - 1;
}
vq_Type vq_op (int opcode, vq_Item args[]) {
    return (unsigned) opcode < sizeof f_commands / sizeof *f_commands ?
        f_commands[opcode].proc(args) : VQ_nil;
}
