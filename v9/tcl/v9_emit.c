/* V9 save a view as binary data, using the Metakit file format
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include <stdlib.h>
#include <string.h>

typedef struct EmitItem {
    intptr_t size;
    const void *data;
} EmitItem;

typedef struct EmitInfo {
    int64_t  position;   /* emit offset, track >2 Gb even on 32b arch */
    Buffer  *itembuf;    /* item buffer */
    Buffer  *colbuf;     /* column buffer */
} EmitInfo;

static void EmitViewData (EmitInfo *eip, V9View t, int describe); /* forward */

static V9Types GetCell (int row, V9Item* pitem) {
    Vector vp = pitem->c.p;
    assert(Head(vp).type->getter != 0);
    return Head(vp).type->getter(row, pitem);
}
    
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
    void *buf = 0;
    if (size > 0) {
        buf = memcpy(malloc(size), data, size);
        EmitBlock(eip, buf, size);
    }
    return buf;
}

static intptr_t EmitBuffer (EmitInfo *eip, Buffer *buf) {
    intptr_t pos = EmitBlock(eip, BufferAsPtr(buf, 0), BufferCount(buf));
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
        result = 0;
    }

    *outsize = bytes;
    return result;
}

static int EmitFixCol (EmitInfo *eip, V9Item column, V9Types type) {
    int r, rows = Head(column.v).count, *tempvec;
    void *buffer;
    intptr_t bufsize;

    switch (type) {
        case V9T_int:
            bufsize = rows * sizeof(int);
            tempvec = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                V9Item cell = column;
                GetCell(r, &cell);
                tempvec[r] = cell.i;
            }
            buffer = PackedIntVec(tempvec, rows, &bufsize);
            free(tempvec); /* TODO: avoid extra copy & malloc */
            break;
        case V9T_long:
            bufsize = rows * sizeof(int64_t);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                V9Item cell = column;
                GetCell(r, &cell);
                ((int64_t*) buffer)[r] = cell.l;
            }
            break;
        case V9T_float:
            bufsize = rows * sizeof(float);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                V9Item cell = column;
                GetCell(r, &cell);
                ((float*) buffer)[r] = cell.f;
            }
            break;
        case V9T_double:
            bufsize = rows * sizeof(double);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r) {
                V9Item cell = column;
                GetCell(r, &cell);
                ((double*) buffer)[r] = cell.d;
            }
            break;
        default:
            assert(0);
            return 0;
    }
    EmitPair(eip, EmitBlock(eip, buffer, bufsize));
    return bufsize != 0;
}

static void EmitVarCol (EmitInfo *eip, V9Item column, int istext, int rows) {
    int r, bytes;
    intptr_t buflen;
    Buffer buffer;
    Vector sizes = NewDataVec(V9T_int, rows);
    int *sizevec = (void*) sizes;

    InitBuffer(&buffer);

    if (istext)
        for (r = 0; r < rows; ++r) {
            V9Item cell = column;
            GetCell(r, &cell);
            bytes = strlen(cell.s);
            if (bytes > 0)
                AddToBuffer(&buffer, cell.s, ++bytes);
            sizevec[r] = bytes;
        }
    else
        for (r = 0; r < rows; ++r) {
            V9Item cell = column;
            GetCell(r, &cell);
            AddToBuffer(&buffer, cell.p, cell.c.i);
            sizevec[r] = cell.c.i;
        }

    buflen = BufferCount(&buffer);
    EmitPair(eip, EmitBuffer(eip, &buffer));    
    if (buflen > 0) {
        V9Item cell;
        cell.p = sizes;
        EmitFixCol(eip, cell, 1);
    }
    DecRef(sizes);

    EmitVarInt(eip, 0); /* no memos */
}
    
static void EmitSubCol (EmitInfo *eip, V9Item column, int describe, int rows) {
    int r;
    Buffer newcolbuf;
    Buffer *origcolbuf;
    origcolbuf = eip->colbuf;
    eip->colbuf = &newcolbuf;
    InitBuffer(eip->colbuf);
    for (r = 0; r < rows; ++r) {
        V9Item cell = column;
        V9Types type = GetCell(r, &cell);
        assert(type == V9T_view); UNUSED(type);
        EmitViewData(eip, cell.v, describe);
    }
    eip->colbuf = origcolbuf;
    EmitPair(eip, EmitBuffer(eip, &newcolbuf));  
}

static void EmitCols (EmitInfo *eip, V9View view) {
    int rows = Head(view).count;
    EmitVarInt(eip, rows);
    if (rows > 0) {
        int c;
        V9View subv, meta = view->meta;
        for (c = 0; c < Head(meta).count; ++c) {                          
            V9Types type = V9_GetInt(meta, c, 1);
            switch (type) {
                case V9T_int:
                case V9T_long:
                case V9T_float:
                case V9T_double:
                    EmitFixCol(eip, view->col[c], type); 
                    break;
                case V9T_string:
                    EmitVarCol(eip, view->col[c], 1, rows);
                    break;
                case V9T_bytes:
                    EmitVarCol(eip, view->col[c], 0, rows);
                    break;
                case V9T_view:
                    subv = V9_GetView(meta, c, 2);
                    EmitSubCol(eip, view->col[c], Head(subv).count == 0, rows);
                    break;
                default: assert(0);
            }
        }
    }
}

static void EmitViewData (EmitInfo *eip, V9View view, int describe) {
    EmitVarInt(eip, 0);

    if (describe) {
        int len = V9_MetaAsDescLength(view->meta);
        char* buf = malloc(len + 1);
        V9_MetaAsDesc(view->meta, buf);
        EmitVarInt(eip, len);
        AddToBuffer(eip->colbuf, buf, len);
        free(buf);
    }

    EmitCols(eip, view);
}

static void SetBigEndian32 (char* dest, intptr_t value) {
    dest[0] = (char) (value >> 24);
    dest[1] = (char) (value >> 16);
    dest[2] = (char) (value >> 8);
    dest[3] = (char) value;
}

static intptr_t EmitComplete (EmitInfo *eip, V9View view) {
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

    EmitViewData(eip, view, 1);

    rootpos = EmitBuffer(eip, eip->colbuf);  
    eip->colbuf = 0;

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

int V9_ViewEmit (V9View view, void *aux, void *(*fini)(void*,intptr_t), void* (*fdat)(void*,const void*,intptr_t)) {
    Buffer buffer;
    EmitInfo einfo;
    
    InitBuffer(&buffer);
    einfo.itembuf = &buffer;

    intptr_t bytes = EmitComplete(&einfo, view);

    int numitems = BufferCount(&buffer) / sizeof(EmitItem);
    EmitItem* items = BufferAsPtr(&buffer, 1);

    if (fini != 0)
        aux = fini(aux, bytes);

    int i;
    for (i = 0; i < numitems; ++i) {
        if (aux != 0)
            aux = fdat(aux, items[i].data, items[i].size);
        free((void*) items[i].data);
    }
    
    if (aux == 0)
        bytes = 0;
    
    ReleaseBuffer(&buffer, 0);
    return bytes;
}
