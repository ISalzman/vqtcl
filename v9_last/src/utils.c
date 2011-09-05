#include "vq.h"
#include "defs.h"

void releaseColumn (vColPtr cptr) {
    assert(cptr->refs == 0);
    free(cptr);
}

vColPtr incRef (vColPtr cptr) {
    ++cptr->refs;
    return cptr;
}

void decRef (vColPtr cptr) {
    if (cptr != 0 && --cptr->refs == 0)
        (cptr->dispatch->cleaner)(cptr);
}

vType getItem (vView vw, int row, int col, vAny *val) {
    *val = vw->columns[col];
    return (val->c->dispatch->getter)(row, val);
}
int getIntItem (vView vw, int row, int col) {
    vAny vec;
    vType type = getItem(vw, row, col, &vec);
    assert(type == v_INT || type == v_POS);
    return type == v_INT || type == v_POS ? vec.i : -1;
}

void setColumn (vView vw, int col, vColPtr cptr) {
    assert(vw->columns[col].c != cptr);
    decRef(vw->columns[col].c);
    vw->columns[col].c = incRef(cptr);
    /* TODO set parent */
}
void setItem (vView vw, int row, int col, const vAny *val) {
    vAny vec = vw->columns[col];
    (vec.c->dispatch->setter)(vec.c, row, vec.pair.two.j, val);
}
void setIntItem (vView vw, int row, int col, int val) {
    vAny cell;
    assert(vCOLTYPE(vw, col) == v_INT);
    cell.i = val;
    setItem(vw, row, col, &cell);
}
void setStringItem (vView vw, int row, int col, const char *val) {
    vAny cell;
    assert(vCOLTYPE(vw, col) == v_STRING);
    cell.s = val;
    setItem(vw, row, col, &cell);
}
void setViewItem (vView vw, int row, int col, vView val) {
    vAny cell;
    assert(vCOLTYPE(vw, col) == v_VIEW);
    cell.v = val;
    setItem(vw, row, col, &cell);
}
static void setMetaRow (vView m, int row, const char *nm, int tp, vView sv) {
    setStringItem(m, row, 0, nm);
    setIntItem(m, row, 1, tp);
    setViewItem(m, row, 2, sv);
}

vView newView (vView meta, int rows) {
    int c;
    vEnv *env = vENV(meta);
    int cols = vROWS(meta);
    vView result = (vView) newDataColumn(env, v_VIEW, 1 + cols);
    vMETA(result) = (vView) incRef((vColPtr) meta);
    vROWS(result) = rows;
    for (c = 0; c < cols; ++c) {
        vType type = getIntItem(meta, c, 1);
        setColumn(result, c, newDataColumn(env, type, rows));
    }
    return result;
}

vEnv *initEnv (void *aux) {
    vEnv *env = calloc(1, sizeof(vEnv));
    vView mmeta = (vView) newDataColumn(env, v_VIEW, 1 + 3 + 1);
    vMETA(mmeta) = (vView) incRef((vColPtr) mmeta);
    vROWS(mmeta) = 3;
    mmeta->columns[3].p = env;
    setColumn(mmeta, 0, newDataColumn(env, v_STRING, 3));
    setColumn(mmeta, 1, newDataColumn(env, v_INT, 3));
    setColumn(mmeta, 2, newDataColumn(env, v_VIEW, 3));
    setMetaRow(mmeta, 0, "name", v_STRING, mmeta);
    setMetaRow(mmeta, 1, "type", v_INT, mmeta);
    setMetaRow(mmeta, 2, "subv", v_VIEW, mmeta);
    env->aux = aux;
    env->empty = newView(mmeta, 0);
    setViewItem(mmeta, 0, 2, env->empty);
    setViewItem(mmeta, 1, 2, env->empty);
    return env;
}
