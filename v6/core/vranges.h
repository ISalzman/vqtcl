/*  Support for ranges and missing values.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#ifndef VQ_RANGES_H
#define VQ_RANGES_H 1

#include "v_defs.h"

void* (VecInsert) (Vector *vecp, int off, int cnt);
void* (VecDelete) (Vector *vecp, int off, int cnt);

void* (RangeFlip) (Vector *vecp, int off, int count);
int (RangeLocate) (Vector v, int off, int *offp);
void (RangeInsert) (Vector *vecp, int off, int count, int mode);
void (RangeDelete) (Vector *vecp, int off, int count);

vq_Type (RflipCmd_OII) (vq_Item a[]);
vq_Type (RlocateCmd_OI) (vq_Item a[]);
vq_Type (RinsertCmd_OIII) (vq_Item a[]);
vq_Type (RdeleteCmd_OII) (vq_Item a[]);

#endif