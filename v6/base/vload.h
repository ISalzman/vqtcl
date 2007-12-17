/*  Load views from a Metakit-format memory map.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_LOAD_H
#define VQ_LOAD_H 1

#include "v_defs.h"

vq_View (AsMetaVop) (const char *desc);
vq_View (OpenVop) (const char *filename);

#endif
