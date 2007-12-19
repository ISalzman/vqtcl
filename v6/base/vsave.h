/*  Save Metakit-format views to memory or file.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#ifndef VQ_SAVE_H
#define VQ_SAVE_H 1

#include "v_defs.h"

typedef void *(*SaveInitFun)(void*,intptr_t);
typedef void *(*SaveDataFun)(void*,const void*,intptr_t);

intptr_t (ViewSave) (vq_View t, void *aux, SaveInitFun fi, SaveDataFun fd);

void (MetaAsDesc) (vq_View meta, Buffer *buffer);
vq_Type (Meta2DescCmd_V) (vq_Cell a[]);
vq_Type (EmitCmd_V) (vq_Cell a[]);

#endif
