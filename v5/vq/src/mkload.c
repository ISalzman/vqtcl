/* Load views from a Metakit-format memory map */

#include "defs.h"

#define MF_Data(x) ((x)->o.a.s)
#define MF_Length(x) ((x)->o.b.i)

static vq_Table MapSubview (Vector map, int offset, vq_Table meta, vq_Table base) {
    return 0;
}

static int BigEndianInt32 (const char *p) {
    const unsigned char *up = (const unsigned char*) p;
    return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

vq_Table MappedToTable (Vector map, vq_Table base) {
    int i, t[4];
    int datalen, rootoff;
    
    if (MF_Length(map) <= 24 || *(MF_Data(map) + MF_Length(map) - 16) != '\x80')
        return NULL;
        
    for (i = 0; i < 4; ++i)
        t[i] = BigEndianInt32(MF_Data(map) + MF_Length(map) - 16 + i * 4);
        
    datalen = t[1] + 16;
    rootoff = t[3];

    if (rootoff < 0) {
        const int mask = 0x7FFFFFFF; 
        datalen = (datalen & mask) + ((int) ((t[0] & 0x7FF) << 16) << 15);
        rootoff = (rootoff & mask) + (datalen & ~mask);
        /* FIXME: rollover at 2 Gb, prob needs: if (rootoff > datalen) ... */
    }
    
    AdjustMappedFile(map, MF_Length(map) - datalen);
    return MapSubview(map, rootoff, EmptyMetaTable(), base);
}
