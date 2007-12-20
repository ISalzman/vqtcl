/*  Save Metakit-format views to memory or file.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#if VQ_SAVE_H

typedef struct EmitItem {
    intptr_t size;
    const void *data;
} EmitItem;

typedef struct EmitInfo {
    int64_t  position;   /* emit offset, track >2 Gb even on 32b arch */
    Buffer  *itembuf;    /* item buffer */
    Buffer  *colbuf;     /* column buffer */
} EmitInfo;

static void EmitView (EmitInfo *eip, vq_View t, int describe); /* forward */
    
/* -------------------------------------------------- META DESCRIPTIONS ----- */

void MetaAsDesc (vq_View meta, Buffer *buffer) {
    int type, r, rows = vCount(meta);
    const char *name;
    char buf [30];
    vq_View subt;

    for (r = 0; r < rows; ++r) {
        if (r > 0)
            ADD_CHAR_TO_BUF(*buffer, ',');

        name = vq_getString(meta, r, 0, "");
        type = vq_getInt(meta, r, 1, VQ_nil);
        subt = vq_getView(meta, r, 2, 0);
        assert(subt != 0);

        AddToBuffer(buffer, name, strlen(name));
        if (type == VQ_view && vCount(subt) > 0) {
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

/* -------------------------------------------------- METAKIT DATA SAVE ----- */

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

static int TopBit (int v) {
#define Vn(x) (v < (1<<x))
    return Vn(16) ? Vn( 8) ? Vn( 4) ? Vn( 2) ? Vn( 1) ? v-1 : 1
                                             : Vn( 3) ?  2 :  3
                                    : Vn( 6) ? Vn( 5) ?  4 :  5
                                             : Vn( 7) ?  6 :  7
                           : Vn(12) ? Vn(10) ? Vn( 9) ?  8 :  9
                                             : Vn(11) ? 10 : 11
                                    : Vn(14) ? Vn(13) ? 12 : 13
                                             : Vn(15) ? 14 : 15
                  : Vn(24) ? Vn(20) ? Vn(18) ? Vn(17) ? 16 : 17
                                             : Vn(19) ? 18 : 19
                                    : Vn(22) ? Vn(21) ? 20 : 21
                                             : Vn(23) ? 22 : 23
                           : Vn(28) ? Vn(26) ? Vn(25) ? 24 : 25
                                             : Vn(27) ? 26 : 27
                                    : Vn(30) ? Vn(29) ? 28 : 29
                                             : Vn(31) ? 30 : 31;
#undef Vn
}
static int MinWidth (int lo, int hi) {
    lo = lo > 0 ? 0 : "444444445555555566666666666666666"[TopBit(~lo)+1] & 7;
    hi = hi < 0 ? 0 : "012334445555555566666666666666666"[TopBit(hi)+1] & 7;
    return lo > hi ? lo : hi;
}
static int* PackedIntVec (const int *data, int rows, intptr_t *outsize) {
    int r, width, lo = 0, hi = 0, bits, *result;
    const int *limit;
    intptr_t bytes;

    for (r = 0; r < rows; ++r) {
        if (data[r] < lo) lo = data[r];
        if (data[r] > hi) hi = data[r];
    }

    width = MinWidth(lo, hi);

    if (width >= 6) {
        bytes = rows * (1 << (width-4));
        result = malloc(bytes);
        memcpy(result, data, bytes);
    } else if (rows > 0 && width > 0) {
        if (rows < 5 && width < 4) {
            static char fudges[3][4] = {    /* n:    1:  2:  3:  4: */
                {1,1,1,1},          /* 1-bit entries:    1b  2b  3b  4b */
                {1,1,1,1},          /* 2-bit entries:    2b  4b  6b  8b */
                {1,1,2,2},          /* 4-bit entries:    4b  8b 12b 16b */
            };
            static char widths[3][4] = {    /* n:    1:  2:  3:  4: */
                {3,3,2,2},          /* 1-bit entries:    4b  4b  2b  2b */
                {3,3,2,2},          /* 2-bit entries:    4b  4b  2b  2b */
                {3,3,3,3},          /* 4-bit entries:    4b  4b  4b  4b */
            };
            bytes = fudges[width-1][rows-1];
            width = widths[width-1][rows-1];
        } else
            bytes = (((intptr_t) rows << width) + 14) >> 4; /* round up */
            
        result = malloc(bytes);
        if (width < 4)
            memset(result, 0, bytes);

        limit = data + rows;
        bits = 0;

        switch (width) {
            case 1: { /* 1 bit, 8 per byte */
                char* q = (char*) result;
                while (data < limit) {
                    *q |= (*data++ & 1) << bits; ++bits; q += bits >> 3; 
                    bits &= 7;
                }
                break;
            }
            case 2: { /* 2 bits, 4 per byte */
                char* q = (char*) result;
                while (data < limit) {
                    *q |= (*data++ & 3) << bits; bits += 2; q += bits >> 3; 
                    bits &= 7;
                }
                break;
            }
            case 3: { /* 4 bits, 2 per byte */
                char* q = (char*) result;
                while (data < limit) {
                    *q |= (*data++ & 15) << bits; bits += 4; q += bits >> 3;
                    bits &= 7;
                }
                break;
            }
            case 4: { /* 1-byte (char) */
                char* q = (char*) result;
                while (data < limit)
                    *q++ = (char) *data++;
                break;
            }
            case 5: { /* 2-byte (short) */
                short* q = (short*) result;
                while (data < limit)
                    *q++ = (short) *data++;
                break;
            }
        }
    } else {
        bytes = 0;
        result = NULL;
    }

    *outsize = bytes;
    return result;
}

static int EmitFixCol (EmitInfo *eip, vq_Cell column, vq_Type type) {
    int r, rows = vCount(column.o.a.v), *tempvec;
    void *buffer;
    intptr_t bufsize;

    switch (type) {
        case VQ_int:
            bufsize = rows * sizeof(int);
            tempvec = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Cell item = column;
                GetItem(r, &item);
                tempvec[r] = item.o.a.i;
            }
            buffer = PackedIntVec(tempvec, rows, &bufsize);
            free(tempvec); /* TODO: avoid extra copy & malloc */
            break;
        case VQ_long:
            bufsize = rows * sizeof(int64_t);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Cell item = column;
                GetItem(r, &item);
                ((int64_t*) buffer)[r] = item.w;
            }
            break;
        case VQ_float:
            bufsize = rows * sizeof(float);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Cell item = column;
                GetItem(r, &item);
                ((float*) buffer)[r] = item.o.a.f;
            }
            break;
        case VQ_double:
            bufsize = rows * sizeof(double);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                vq_Cell item = column;
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

static void EmitVarCol (EmitInfo *eip, vq_Cell column, int istext, int rows) {
    int r, bytes;
    intptr_t buflen;
    Buffer buffer;
    Vector sizes = vq_retain(AllocDataVec(VQ_int, rows));
    int *sizevec = (void*) sizes;

    InitBuffer(&buffer);

    if (istext)
        for (r = 0; r < rows; ++r) {
            vq_Cell item = column;
            GetItem(r, &item);
            bytes = strlen(item.o.a.s);
            if (bytes > 0)
                AddToBuffer(&buffer, item.o.a.s, ++bytes);
            sizevec[r] = bytes;
        }
    else
        for (r = 0; r < rows; ++r) {
            vq_Cell item = column;
            GetItem(r, &item);
            AddToBuffer(&buffer, item.o.a.p, item.o.b.i);
            sizevec[r] = item.o.b.i;
        }

    buflen = BufferFill(&buffer);
    EmitPair(eip, EmitBuffer(eip, &buffer));    
    if (buflen > 0) {
        vq_Cell item;
        item.o.a.v = sizes;
        EmitFixCol(eip, item, 1);
    }
    vq_release(sizes);

    EmitVarInt(eip, 0); /* no memos */
}
    
static void EmitSubCol (EmitInfo *eip, vq_Cell column, int describe, int rows) {
    int r;
    Buffer newcolbuf;
    Buffer *origcolbuf;
    origcolbuf = eip->colbuf;
    eip->colbuf = &newcolbuf;
    InitBuffer(eip->colbuf);
    for (r = 0; r < rows; ++r) {
        vq_Cell item = column;
        vq_Type type = GetItem (r, &item);
        assert(type == VQ_view); VQ_UNUSED(type);
        EmitView(eip, item.o.a.v, describe);
    }
    eip->colbuf = origcolbuf;
    EmitPair(eip, EmitBuffer(eip, &newcolbuf));  
}

static void EmitCols (EmitInfo *eip, vq_View view) {
    int rows = vCount(view);
    EmitVarInt(eip, rows);
    if (rows > 0) {
        int c;
        vq_View subt, meta = vMeta(view);
        for (c = 0; c < vCount(meta); ++c) {                          
            vq_Type type = vq_getInt(meta, c, 1, VQ_nil);
            switch (type) {
                case VQ_int:
                case VQ_long:
                case VQ_float:
                case VQ_double:
                    EmitFixCol(eip, view[c], type); 
                    break;
                case VQ_string:
                    EmitVarCol(eip, view[c], 1, rows);
                    break;
                case VQ_bytes:
                    EmitVarCol(eip, view[c], 0, rows);
                    break;
                case VQ_view:
                    subt = vq_getView(meta, c, 2, 0);
                    EmitSubCol(eip, view[c], vCount(subt) == 0, rows);
                    break;
                default: assert(0);
            }
        }
    }
}

static void EmitView (EmitInfo *eip, vq_View view, int describe) {
    EmitVarInt(eip, 0);

    if (describe) {
        int cnt = 0;
        char *ptr = NULL;
        struct Buffer desc;
        InitBuffer(&desc);
        MetaAsDesc(vMeta(view), &desc);
        EmitVarInt(eip, BufferFill(&desc));
        while (NextBuffer(&desc, &ptr, &cnt))
            AddToBuffer(eip->colbuf, ptr, cnt);
    }

    EmitCols(eip, view);
}

static void SetBigEndian32 (char* dest, intptr_t value) {
    dest[0] = (char) (value >> 24);
    dest[1] = (char) (value >> 16);
    dest[2] = (char) (value >> 8);
    dest[3] = (char) value;
}

static intptr_t EmitComplete (EmitInfo *eip, vq_View view) {
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

    EmitView(eip, view, 1);

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

intptr_t ViewSave (vq_View view, void *aux, SaveInitFun initfun, SaveDataFun datafun) {
    int i, numitems;
    intptr_t bytes;
    Buffer buffer;
    EmitItem *items;
    EmitInfo einfo;
    
    InitBuffer(&buffer);
    
    einfo.itembuf = &buffer;

    bytes = EmitComplete(&einfo, view);

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

/* -------------------------------------------------- OPERATOR WRAPPERS ----- */

/*
vq_Type Meta2DescCmd_T (vq_Cell a[]) {
    Buffer buffer;
    InitBuffer(&buffer);
    MetaAsDesc(vMeta(a[0].o.a.v), &buffer);
    ADD_CHAR_TO_BUF(buffer, 0);
    a[0].o.a.s = BufferAsPtr(&buffer, 1);
    a->o.a.p = ItemAsObj(VQ_string, a[0]);
    ReleaseBuffer(&buffer, 0);
    return VQ_objref;
}
*/

#endif
