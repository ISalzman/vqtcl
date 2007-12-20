/*  Utility code.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#if VQ_UTIL_H

int CharAsType (char c) {
    const char *p = strchr(VQ_TYPES, c & ~0x20);
    int type = p != 0 ? p - VQ_TYPES : VQ_nil;
    if (c & 0x20)
        type |= VQ_NULLABLE;
    return type;
}

int StringAsType (const char *str) {
    int type = CharAsType(*str);
    while (*++str != 0)
        if ('a' <= *str && *str <= 'z')
        type |= 1 << (*str - 'a' + 5);
    return type;
}

const char* TypeAsString (int type, char *buf) {
    char c, *p = buf; /* buffer should have room for at least 28 bytes */
    *p++ = VQ_TYPES[type&VQ_TYPEMASK];
    if (type & VQ_NULLABLE)
        p[-1] |= 0x20;
    for (c = 'a'; c <= 'z'; ++c)
        if (type & (1 << (c - 'a' + 5)))
            *p++ = c;
    *p = 0;
    return buf;
}

void IndirectCleaner (Vector v) {
    vq_release(vMeta(v));
    FreeVector(v);
}

vq_View IndirectView (vq_View meta, Dispatch *vtabp, int rows, int extra) {
    int i, cols = vCount(meta);
    vq_View t = AllocVector(vtabp, cols * sizeof *t + extra);
    assert(vtabp->prefix >= 3);
    vMeta(t) = vq_retain(meta);
    vData(t) = t + cols;
    vCount(t) = rows;
    for (i = 0; i < cols; ++i) {
        t[i].o.a.v = t;
        t[i].o.b.i = i;
    }
    return t;
}

#endif
