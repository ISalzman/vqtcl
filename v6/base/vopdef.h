/*  Additional view operators.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_OPDEF_H
#define VQ_OPDEF_H 1

#include "v_defs.h"

vq_View (StepVop) (int rows, int start, int step, int rate, const char *name);
vq_View (PassVop) (vq_View v);
const char* (TypeVop) (vq_View v);
vq_View (ViewVop) (vq_View v, vq_View m);
vq_View (RowMapVop) (vq_View v, vq_View map);
vq_View (ColMapVop) (vq_View v, vq_View map);
vq_View (RowCatVop) (Vector map);

#endif
