/*  Load views from a Metakit-format memory map.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_LOAD_H
#define VQ_LOAD_H 1

#include "v_defs.h"

vq_View (DescToMeta) (const char *desc, int length);
vq_View (MapToView) (Vector map);

vq_Type (Desc2MetaCmd_S) (vq_Item a[]);
vq_Type (OpenCmd_S) (vq_Item a[]);

vq_Type (LoadCmd_O) (vq_Item a[]);

#endif
