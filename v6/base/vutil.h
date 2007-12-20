/*  Utility code.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_UTIL_H
#define VQ_UTIL_H 1

#include "v_defs.h"

int (CharAsType) (char c);
int (StringAsType) (const char *str);
const char* (TypeAsString) (int type, char *buf);

void (IndirectCleaner) (Vector v);
vq_View (IndirectView) (vq_View meta, Dispatch *vtab, int rows, int extra);

#endif
