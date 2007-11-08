/* Save Metakit-format views to memory or file */

#include "defs.h"

#if VQ_MOD_MKSAVE

#include <stdio.h>
#include <string.h>

static void MetaAsDesc (vq_Table meta, Buffer *buffer) {
    int type, r, rows = vCount(meta);
    const char *name;
    char buf [30];
    vq_Table subt;

    for (r = 0; r < rows; ++r) {
        if (r > 0)
            ADD_CHAR_TO_BUF(*buffer, ',');

        name = Vq_getString(meta, r, 0, "");
        type = Vq_getInt(meta, r, 1, VQ_nil);
        subt = Vq_getTable(meta, r, 2, 0);
        assert(subt != 0);

        AddToBuffer(buffer, name, strlen(name));
        if (type == VQ_table && vCount(subt) > 0) {
            ADD_CHAR_TO_BUF(*buffer, '[');
            /*MetaAsDesc(subt, buffer);*/
            ADD_CHAR_TO_BUF(*buffer, ']');
        } else {
            ADD_CHAR_TO_BUF(*buffer, ':');
            TypeAsString(type, buf);
            AddToBuffer(buffer, buf, strlen(buf));
        }
    }
}

vq_Type Meta2DescCmd_T (vq_Item a[]) {
    Buffer buffer;
    InitBuffer(&buffer);
    MetaAsDesc(vMeta(a[0].o.a.m), &buffer);
    ADD_CHAR_TO_BUF(buffer, 0);
    a[0].o.a.s = BufferAsPtr(&buffer, 1);
    a->o.a.p = ItemAsObj(VQ_string, a[0]);
    ReleaseBuffer(&buffer, 0);
    return VQ_object;
}

vq_Type EmitCmd_T (vq_Item a[]) {
    return VQ_nil;
}

#endif
