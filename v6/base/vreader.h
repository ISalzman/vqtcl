/*  Memory-mapped file and and reader interface.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_READER_H
#define VQ_READER_H 1

#include "v_defs.h"

Vector (OpenMappedFile) (const char *filename);
Vector (OpenMappedBytes) (const void *data, int length, Object_p ref);
const char* (AdjustMappedFile) (Vector map, int offset);
Dispatch* (PickIntGetter) (int bits);
Dispatch* (FixedGetter) (int bytes, int rows, int real, int flip);

#endif
