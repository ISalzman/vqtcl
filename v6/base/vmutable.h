/*  Support for mutable views.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_MUTABLE_H
#define VQ_MUTABLE_H 1

#include "v_defs.h"

int (IsMutable) (vq_View t);
vq_View (MutableVop) (vq_View t);

#endif
