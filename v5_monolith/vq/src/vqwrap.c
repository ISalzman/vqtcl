/* Vlerq wrappers around core functions */

#include "vqdefs.h"

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

void Vq_setEmpty (vq_Table t, int row, int col) {
    vq_Item item;
    item.o.a.p = 0;
    vq_set(t, row, col, VQ_nil, item);
}

void Vq_setInt (vq_Table t, int row, int col, int val) {
    vq_Item item;
    item.o.a.i = val;
    vq_set(t, row, col, VQ_int, item);
}

void Vq_setString (vq_Table t, int row, int col, const char *val) {
    vq_Item item;
    item.o.a.s = val;
    vq_set(t, row, col, val != 0 ? VQ_string : VQ_nil, item);
}

void Vq_setTable (vq_Table t, int row, int col, vq_Table val) {
    vq_Item item;
    item.o.a.m = val;
    vq_set(t, row, col, val != 0 ? VQ_table : VQ_nil, item);
}
