/*  Vlerq base implementation.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include "vlerq.h"
#include "vqbase.h"

#include <unistd.h>

vqView vq_init (lua_State *L) {
    return 0;
}

lua_State *vq_state (vqView v) {
    return vwState(v);
}

vqView vq_new (vqView m, int rows) {
    return 0;
}

vqView vq_meta (vqView v) {
    return vwMeta(v);
}

int vq_rows (vqView v) {
    return vwRows(v);
}

int vq_isnil (vqView v, int row, int col) {
    return 0;
}

vqCell vq_get (vqView v, int row, int col, vqType type, vqCell def) {
    return def;
}

vqView vq_set (vqView v, int row, int col, vqType type, vqCell val) {
    return 0;
}

vqView vq_replace (vqView v, int start, int count, vqView data) {
    return 0;
}

