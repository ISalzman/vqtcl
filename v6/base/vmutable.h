/*  Support for mutable views.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#ifndef VQ_MUTABLE_H
#define VQ_MUTABLE_H 1

#include "v_defs.h"

int (IsMutable) (vq_View t);
vq_View (WrapMutable) (vq_View t, Object_p o);

vq_Type (ReplaceCmd_OIIV) (vq_Item a[]);
vq_Type (SetCmd_OIIO) (vq_Item a[]);
vq_Type (UnsetCmd_OII) (vq_Item a[]);

#endif
