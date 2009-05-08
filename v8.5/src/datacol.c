/* Create a basic data vector of the specified type and size */

#include "vq.h"
#include "defs.h"

static vType nilGetter (int row, vAny *ap) {
    vUNUSED(row);
    vUNUSED(ap);
    return v_NIL;
}
static void nilSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(cptr);
    vUNUSED(row);
    vUNUSED(col);
    vUNUSED(ap);
}

static vType intGetter (int row, vAny *ap) {
    int value = ap->c->items.i[row];
    ap->i = value;
    return v_INT;
}
static void intSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    cptr->items.i[row] = ap->i;
}

static vType posGetter (int row, vAny *ap) {
    int value = ap->c->items.i[row];
    ap->i = value;
    return v_POS;
}
static void posSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    cptr->items.i[row] = ap->i;
}

static vType longGetter (int row, vAny *ap) {
    vLong value = ap->c->items.l[row];
    ap->l = value;
    return v_LONG;
}
static void longSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    cptr->items.l[row] = ap->l;
}

static vType floatGetter (int row, vAny *ap) {
    float value = ap->c->items.f[row];
    ap->f = value;
    return v_FLOAT;
}
static void floatSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    cptr->items.f[row] = ap->f;
}

static vType doubleGetter (int row, vAny *ap) {
    double value = ap->c->items.d[row];
    ap->d = value;
    return v_DOUBLE;
}
static void doubleSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    cptr->items.d[row] = ap->d;
}

static void stringCleaner (vColPtr cptr) {
    int i;
    for (i = 0; i < cptr->size; ++i)
        free(cptr->items.p[i]);
    releaseColumn(cptr);
}
static vType stringGetter (int row, vAny *ap) {
    const char *value = ap->c->items.p[row];
    ap->s = value;
    return v_STRING;
}
static void stringSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    free(cptr->items.p[row]);
    cptr->items.p[row] = strcpy(malloc(strlen(ap->s) + 1), ap->s);
}

static void bytesCleaner (vColPtr cptr) {
    int i;
    for (i = 0; i < cptr->size; ++i)
        free(cptr->items.a[i].p);
    releaseColumn(cptr);
}
static vType bytesGetter (int row, vAny *ap) {
    vAny value = ap->c->items.a[row];
    *ap = value;
    return v_BYTES;
}
static void bytesSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    free(cptr->items.a[row].p);
    cptr->items.a[row].p = memcpy(malloc(ap->pair.two.j), ap->p, ap->pair.two.j);
    cptr->items.a[row].pair.two.j = ap->pair.two.j;
}

static void nestedCleaner (vColPtr cptr) {
    int i;
    for (i = 0; i < cptr->size; ++i)
        decRef(cptr->items.a[i].c);
    releaseColumn(cptr);
}
static vType nestedGetter (int row, vAny *ap) {
    vAny value = ap->c->items.a[row];
    *ap = value;
    return v_VIEW;
}
static void nestedSetter (vColPtr cptr, int row, int col, const vAny *ap) {
    vUNUSED(col);
    incRef(ap->c);
    decRef(cptr->items.a[row].c);
    cptr->items.a[row] = *ap;
}

static const vDispatch datavecTypes [] = {
    { "nilvec", v_NIL, 0, 0, 0,
        releaseColumn, nilGetter, nilSetter },
    { "intvec", v_INT, sizeof (int), 0, 0,
        releaseColumn, intGetter, intSetter },
    { "posvec", v_POS, sizeof (int), 0, 0,
        releaseColumn, posGetter, posSetter },
    { "longvec", v_LONG, sizeof (vLong), 0, 0,
        releaseColumn, longGetter, longSetter },
    { "floatvec", v_FLOAT, sizeof (float), 0, 0,
        releaseColumn, floatGetter, floatSetter },
    { "doublevec", v_DOUBLE, sizeof (double), 0, 0,
        releaseColumn, doubleGetter, doubleSetter },
    { "stringvec", v_STRING, sizeof (const char*), 0, 0,
        stringCleaner, stringGetter, stringSetter },
    { "bytesvec", v_BYTES, sizeof (vAny), 0, 0,
        bytesCleaner, bytesGetter, bytesSetter },
    { "nestedvec", v_VIEW, sizeof (vAny), 0, 0,
        nestedCleaner, nestedGetter, nestedSetter },
};

vColPtr newDataColumn (vEnv *env, vType type, int size) {
    const vDispatch *dispatch;
    vColPtr cptr;
    vUNUSED(env);
    assert(type < sizeof datavecTypes / sizeof *datavecTypes);
    dispatch = &datavecTypes[type];
    cptr = calloc(1, sizeof(struct vSequence_s) + size * dispatch->itemsize);
  	cptr->dispatch = dispatch;
    cptr->size = size;
    return cptr;
}
