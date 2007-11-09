/* Save Metakit-format tables to memory or file */

#include "defs.h"

#if VQ_MOD_MKSAVE

typedef struct EmitItem {
    intptr_t size;
    const void *data;
} EmitItem;

typedef struct EmitInfo {
    int64_t  position;   /* emit offset, track >2 Gb even on 32b arch */
    Buffer  *itembuf;    /* item buffer */
    Buffer  *colbuf;     /* column buffer */
} EmitInfo;

static void EmitTable (EmitInfo *eip, vq_Table t, int describe); /* forward */
    
#pragma mark - META DESCRIPTIONS -

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
            MetaAsDesc(subt, buffer);
            ADD_CHAR_TO_BUF(*buffer, ']');
        } else {
            ADD_CHAR_TO_BUF(*buffer, ':');
            TypeAsString(type, buf);
            AddToBuffer(buffer, buf, strlen(buf));
        }
    }
}

#pragma mark - METAKIT DATA SAVE -

static intptr_t EmitBlock (EmitInfo *eip, const void* data, int size) {
    EmitItem item;
    intptr_t pos = eip->position;

    if (size > 0) {
        item.size = size;
        item.data = data;
        AddToBuffer(eip->itembuf, &item, sizeof item);
        eip->position += size;
    } else
        free((void*) data);
    
    return pos;
}

static void *EmitCopy (EmitInfo *eip, const void* data, int size) {
    void *buf = NULL;
    if (size > 0) {
        buf = memcpy(malloc(size), data, size);
        EmitBlock(eip, buf, size);
    }
    return buf;
}

static intptr_t EmitBuffer (EmitInfo *eip, Buffer *buf) {
    intptr_t pos = EmitBlock(eip, BufferAsPtr(buf, 0), BufferFill(buf));
    ReleaseBuffer(buf, 1);
    return pos;
}

static void EmitVarInt (EmitInfo *eip, intptr_t value) {
    int n;
    assert(value >= 0);
    for (n = 7; (value >> n) > 0; n += 7)
        ;
    while ((n -= 7) > 0)
        ADD_ONEC_TO_BUF(*eip->colbuf, (value >> n) & 0x7F);
    ADD_CHAR_TO_BUF(*eip->colbuf, (value & 0x7F) | 0x80);
}

static void EmitPair (EmitInfo *eip, intptr_t offset) {
    intptr_t size = eip->position - offset;
    EmitVarInt(eip, size);
    if (size > 0)
        EmitVarInt(eip, offset);
}

static int EmitFixCol (EmitInfo *eip, vq_Item column, vq_Type type) {
    int r, rows = vCount(column.o.a.m), *tempvec;
    void *buffer;
    intptr_t bufsize;

    switch (type) {
        case VQ_int:
            bufsize = rows * sizeof(int);
            tempvec = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Item item = column;
                GetItem(r, &item);
                tempvec[r] = item.o.a.i;
            }
            buffer = tempvec;
            break;
        case VQ_long:
            bufsize = rows * sizeof(int64_t);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Item item = column;
                GetItem(r, &item);
                ((int64_t*) buffer)[r] = item.w;
            }
            break;
        case VQ_float:
            bufsize = rows * sizeof(float);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Item item = column;
                GetItem(r, &item);
                ((float*) buffer)[r] = item.o.a.f;
            }
            break;
        case VQ_double:
            bufsize = rows * sizeof(double);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Item item = column;
                GetItem(r, &item);
                ((double*) buffer)[r] = item.d;
            }
            break;
        default:
            assert(0);
            return 0;
    }
    EmitPair(eip, EmitBlock(eip, buffer, bufsize));
    return bufsize != 0;
}

static void EmitVarCol (EmitInfo *eip, vq_Item column, int istext, int rows) {
    int r, bytes;
    intptr_t buflen;
    Buffer buffer;
    Vector sizes = AllocDataVec(VQ_int, rows);
    int *sizevec = (void*) sizes;

    InitBuffer(&buffer);

    if (istext)
        for (r = 0; r < rows; ++r) {
            vq_Item item = column;
            GetItem(r, &item);
            bytes = strlen(item.o.a.s);
            if (bytes > 0)
                AddToBuffer(&buffer, item.o.a.s, ++bytes);
            sizevec[r] = bytes;
        }
    else
        for (r = 0; r < rows; ++r) {
            vq_Item item = column;
            GetItem(r, &item);
            AddToBuffer(&buffer, item.o.a.p, item.o.b.i);
            sizevec[r] = item.o.b.i;
        }

    buflen = BufferFill(&buffer);
    EmitPair(eip, EmitBuffer(eip, &buffer));    
    if (buflen > 0) {
        vq_Item item;
        item.o.a.m = sizes;
        vCount(sizes) = rows;
        EmitFixCol(eip, item, 1);
    }

    EmitVarInt(eip, 0); /* no memos */
}
    
static void EmitSubCol (EmitInfo *eip, vq_Item column, int describe, int rows) {
    int r;
    Buffer newcolbuf;
    Buffer *origcolbuf;
    origcolbuf = eip->colbuf;
    eip->colbuf = &newcolbuf;
    InitBuffer(eip->colbuf);
    for (r = 0; r < rows; ++r) {
        vq_Item item = column;
        vq_Type type = GetItem (r, &item);
        assert(type == VQ_table);
        EmitTable(eip, item.o.a.m, describe);
    }
    eip->colbuf = origcolbuf;
    EmitPair(eip, EmitBuffer(eip, &newcolbuf));  
}

static void EmitCols (EmitInfo *eip, vq_Table table) {
    int rows = vCount(table);
    EmitVarInt(eip, rows);
    if (rows > 0) {
        int c;
        vq_Table subt, meta = vMeta(table);
        for (c = 0; c < vCount(meta); ++c) {                          
            vq_Type type = Vq_getInt(meta, c, 1, VQ_nil);
            switch (type) {
                case VQ_int:
                case VQ_long:
                case VQ_float:
                case VQ_double:
                    EmitFixCol(eip, table[c], type); 
                    break;
                case VQ_string:
                    EmitVarCol(eip, table[c], 1, rows);
                    break;
                case VQ_bytes:
                    EmitVarCol(eip, table[c], 0, rows);
                    break;
                case VQ_table:
                    subt = Vq_getTable(meta, c, 2, 0);
                    EmitSubCol(eip, table[c], vCount(subt) == 0, rows);
                    break;
                default: assert(0);
            }
        }
    }
}

static void EmitTable (EmitInfo *eip, vq_Table table, int describe) {
    EmitVarInt(eip, 0);

    if (describe) {
        int cnt;
        char *ptr = NULL;
        struct Buffer desc;
        InitBuffer(&desc);
        MetaAsDesc(vMeta(table), &desc);
        EmitVarInt(eip, BufferFill(&desc));
        while (NextBuffer(&desc, &ptr, &cnt))
            AddToBuffer(eip->colbuf, ptr, cnt);
    }

    EmitCols(eip, table);
}

static void SetBigEndian32 (char* dest, intptr_t value) {
    dest[0] = (char) (value >> 24);
    dest[1] = (char) (value >> 16);
    dest[2] = (char) (value >> 8);
    dest[3] = (char) value;
}

static intptr_t EmitComplete (EmitInfo *eip, vq_Table table) {
    int overflow;
    intptr_t rootpos, tailpos, endpos;
    char tail[16];
    struct Head { short a; char b; char c; char d[4]; } head, *fixup;
    struct Buffer newcolbuf;

    eip->position = 0;

    head.a = 'J' + ('L' << 8);
    head.b = 0x1A;
    head.c = 0;
    fixup = EmitCopy(eip, &head, sizeof head);

    eip->colbuf = &newcolbuf;
    InitBuffer(eip->colbuf);

    EmitTable(eip, table, 1);

    rootpos = EmitBuffer(eip, eip->colbuf);  
    eip->colbuf = NULL;

    if (eip->position != (intptr_t) eip->position)
        return -1; /* fail if >= 2 Gb on machines with a 32-bit address space */
    
    /* large files will have bit 8 of head[3] and bit 8 of tail[12] set */
    tailpos = eip->position;
    endpos = tailpos + sizeof tail;
    overflow = endpos >> 31;

    /* for file sizes > 2 Gb, store bits 41..31 in tail[2..3] */
    SetBigEndian32(tail, 0x80000000 + overflow);
    SetBigEndian32(tail+4, tailpos);
    SetBigEndian32(tail+8, (0x80 << 24) + tailpos - rootpos);
    SetBigEndian32(tail+12, rootpos);
    if (overflow)
        tail[12] |= 0x80;
    
    EmitCopy(eip, tail, sizeof tail);
    
    if (overflow) {
        /* store bits 41..36 in head[3], and bits 35..4 in head[4..7] */
        assert((endpos & 15) == 0);
        fixup->c = 0x80 | ((endpos >> 16) >> 20);
        SetBigEndian32(fixup->d, endpos >> 4);
    } else
        SetBigEndian32(fixup->d, endpos);
        
    return endpos;
}

intptr_t TableSave (vq_Table table, void *aux, SaveInitFun initfun, SaveDataFun datafun) {
    int i, numitems;
    intptr_t bytes;
    Buffer buffer;
    EmitItem *items;
    EmitInfo einfo;
    
    InitBuffer(&buffer);
    
    einfo.itembuf = &buffer;

    bytes = EmitComplete(&einfo, table);

    numitems = BufferFill(&buffer) / sizeof(EmitItem);
    items = BufferAsPtr(&buffer, 1);

    if (initfun != NULL)
        aux = initfun(aux, bytes);

    for (i = 0; i < numitems; ++i) {
        if (aux != NULL)
            aux = datafun(aux, items[i].data, items[i].size);
        free((void*) items[i].data);
    }
    
    if (aux == NULL)
        bytes = 0;
    
    ReleaseBuffer(&buffer, 0);
    return bytes;
}

#pragma mark - OPERATOR WRAPPERS -

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

#endif
