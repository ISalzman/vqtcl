/* Load views from a Metakit-format memory map */

#include "defs.h"

#include <stdlib.h>
#include <string.h>

#pragma mark - META DESCRIPTIONS -

static vq_Table ParseDesc (char **desc, const char **nbuf, int *tbuf, vq_Table *sbuf) {
    char sep, *ptr = *desc;
    const char  **names = nbuf;
    int c, cols = 0, *types = tbuf;
    vq_Table result, empty, *subts = sbuf;
    
    empty = EmptyMetaTable();
    
    for (;;) {
        const char* s = ptr;
        sbuf[cols] = empty;
        tbuf[cols] = VQ_string;
        
        while (strchr(":,[]", *ptr) == 0)
            ++ptr;
        sep = *ptr;
        *ptr = 0;

        if (sep == '[') {
            ++ptr;
            sbuf[cols] = ParseDesc(&ptr, nbuf+cols, tbuf+cols, sbuf+cols);
            tbuf[cols] = VQ_table;
            sep = *++ptr;
        } else if (sep == ':') {
            tbuf[cols] = CharAsType(*++ptr);
            sep = *++ptr;
        }
        
        nbuf[cols++] = s;
        if (sep != ',')
            break;
            
        ++ptr;
    }
    
    *desc = ptr;

    result = vq_new(vMeta(empty), 3 * sizeof(void*));
    vCount(result) = cols;
    result[0].o.a.m = vq_retain(AllocDataVec(VQ_string, cols));
    result[1].o.a.m = vq_retain(AllocDataVec(VQ_int, cols));
    result[2].o.a.m = vq_retain(AllocDataVec(VQ_table, cols));

    for (c = 0; c < cols; ++c) {
        Vq_setString(result, c, 0, names[c]);
        Vq_setInt(result, c, 1, types[c]);
        Vq_setTable(result, c, 2, subts[c]);
    }
    
    return result;
}
vq_Table DescToMeta (const char *desc, int length) {
    int i, bytes, limit = 1;
    void *buffer;
    const char **nbuf;
    int *tbuf;
    vq_Table *sbuf, meta;
    char *dbuf;
    
    if (length < 0)
        length = strlen(desc);
        
    /* find a hard upper bound for the buffer requirements */
    for (i = 0; i < length; ++i)
        if (desc[i] == ',' || desc[i] == '[')
            ++limit;
    
    bytes = limit * (2 * sizeof(void*) + sizeof(int)) + length + 1;
    buffer = malloc(bytes);
    nbuf = buffer;
    sbuf = (void*) (nbuf + limit);
    tbuf = (void*) (sbuf + limit);
    dbuf = memcpy(tbuf + limit, desc, length);
    dbuf[length] = ']';
    
    meta = ParseDesc(&dbuf, nbuf, tbuf, sbuf);
    
    free(buffer);
    return meta;
}

#pragma mark - METAKIT UTILITY CODE -

#define MF_Data(x) ((x)->o.a.s)
#define MF_Length(x) ((x)->o.b.i)

#if 0
static int IsReversedEndian(Vector map) {
#ifdef VQ_BIG_ENDIAN
    return *MF_Data(map) == 'J';
#else
    return *MF_Data(map) == 'L';
#endif
}
static int GetVarInt (const char **nextp) {
    int8_t b;
    int v = 0;
    do {
        b = *(*nextp)++;
        v = (v << 7) + b;
    } while (b >= 0);
    return v + 128;
}
static int GetVarPair (const char **nextp) {
    int n = GetVarInt(nextp);
    if (n > 0 && GetVarInt(nextp) == 0)
        *nextp += n;
    return n;
}
#endif

#pragma mark - METAKIT DATA READER -

static vq_Table MapSubview (Vector map, int offset, vq_Table meta) {
    return 0;
}

static int BigEndianInt32 (const char *p) {
    const uint8_t *up = (const uint8_t*) p;
    return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

vq_Table MapToTable (Vector map) {
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
    return MapSubview(map, rootoff, EmptyMetaTable());
}
