/*  Additional view operators.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_OPDEF_H
#define VQ_OPDEF_H 1

#include "v_defs.h"

vq_View (StepView) (int rows, int start, int step, int rate, const char *name);
vq_View (PassView) (vq_View v);

#endif
