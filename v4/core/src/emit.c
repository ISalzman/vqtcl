/*
 * emit.c - Implementation of file output commands.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intern.h"
#include "wrap_gen.h"

typedef enum EmitTypes {
    ET_mem
} EmitTypes;

typedef struct EmitItem {
    EmitTypes type;
    Int_t size;
    const void *data;
} EmitItem, *EmitItem_p;

typedef struct EmitInfo {
    int64_t         position;   /* emit offset, track >2 Gb even on 32b arch */
    struct Buffer  *itembuf;    /* item buffer */
    struct Buffer  *colbuf;     /* column buffer */
    int             diff;       /* true if emitting differences */
} EmitInfo, *EmitInfo_p;

static void EmitView (EmitInfo_p eip, View_p view, int describe); /* forward */

static Int_t EmitBlock (EmitInfo_p eip, const void* data, int size) {
    EmitItem item;
    Int_t pos = eip->position;

    if (size > 0) {
        item.type = ET_mem;
        item.size = size;
        item.data = data;
        AddToBuffer(eip->itembuf, &item, sizeof item);

        eip->position += size;
    } else
        free((void*) data);
    
    return pos;
}

static void *EmitCopy (EmitInfo_p eip, const void* data, int size) {
    void *buf = NULL;
    if (size > 0) {
        buf = memcpy(malloc(size), data, size);
        EmitBlock(eip, buf, size);
    }
    return buf;
}

static Int_t EmitBuffer (EmitInfo_p eip, Buffer_p buf) {
    Int_t pos = EmitBlock(eip, BufferAsPtr(buf, 0), BufferFill(buf));
    ReleaseBuffer(buf, 1);
    return pos;
}

static void EmitAlign (EmitInfo_p eip) {
    if (eip->position >= 1024 * 1024)
        EmitCopy(eip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 15 & - eip->position);
}

void MetaAsDesc (View_p meta, Buffer_p buffer) {
    int r, rows = ViewSize(meta);
    Column names, types, subvs;
    char type;
    const char *name;
    View_p subv;

    names = ViewCol(meta, MC_name);
    types = ViewCol(meta, MC_type);
    subvs = ViewCol(meta, MC_subv);

    for (r = 0; r < rows; ++r) {
        if (r > 0)
            AddToBuffer(buffer, ",", 1);

        name = GetColItem(r, names, IT_string).s;
        type = *GetColItem(r, types, IT_string).s;
        subv = GetColItem(r, subvs, IT_view).v;

        AddToBuffer(buffer, name, strlen(name));
        if (type == 'V' && ViewSize(subv) > 0) {
            AddToBuffer(buffer, "[", 1);
            MetaAsDesc(subv, buffer);
            AddToBuffer(buffer, "]", 1);
        } else {
            AddToBuffer(buffer, ":", 1);
            AddToBuffer(buffer, &type, 1);
        }
    }
}

static void EmitVarInt (EmitInfo_p eip, Int_t value) {
    int n;

#if 0
    if (value < 0) {
        ADD_ONEC_TO_BUF(*eip->colbuf, 0);
        value = ~value;
    }
#endif

    Assert(value >= 0);
    for (n = 7; (value >> n) > 0; n += 7)
        ;
    while ((n -= 7) > 0)
        ADD_ONEC_TO_BUF(*eip->colbuf, (value >> n) & 0x7F);
    ADD_CHAR_TO_BUF(*eip->colbuf, (value & 0x7F) | 0x80);
}

static void EmitPair (EmitInfo_p eip, Int_t offset) {
    Int_t size;

    size = eip->position - offset;
    EmitVarInt(eip, size);
    if (size > 0)
        EmitVarInt(eip, offset);
}

static int MinWidth (int lo, int hi) {
    lo = lo > 0 ? 0 : "444444445555555566666666666666666"[TopBit(~lo)+1] & 7;
    hi = hi < 0 ? 0 : "012334445555555566666666666666666"[TopBit(hi)+1] & 7;
    return lo > hi ? lo : hi;
}

static int *PackedIntVec (const int *data, int rows, Int_t *outsize) {
    int r, width, lo = 0, hi = 0, bits, *result;
    const int *limit;
    Int_t bytes;

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
            bytes = (((Int_t) rows << width) + 14) >> 4; /* round up */
            
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

static int EmitFixCol (EmitInfo_p eip, Column column, ItemTypes type) {
    int r, rows, *tempvec;
    void *buffer;
    Int_t bufsize;

    rows = column.seq->count;

    switch (type) {

        case IT_int:
            bufsize = rows * sizeof(int);
            tempvec = malloc(bufsize);
            for (r = 0; r < rows; ++r)
                tempvec[r] = GetColItem(r, column, type).i;
            buffer = PackedIntVec(tempvec, rows, &bufsize);
            free((char*) tempvec); /* TODO: avoid extra copy & malloc */
            
            /* try to compress the bitmap, but only in diff save mode */
            if (eip->diff && rows >= 128 && rows == bufsize * 8) {
                int ebits;

                tempvec = (void*) Bits2elias(buffer, rows, &ebits);

                /* only keep compressed form if under 80% of plain bitmap */
                if (ebits + ebits/4 < rows) {
                    free(buffer);
                    buffer = tempvec;
                    bufsize = (ebits + 7) / 8;
                } else
                    free((char*) tempvec);
            }
            
            break;

        case IT_wide:
            bufsize = rows * sizeof(int64_t);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r)
                ((int64_t*) buffer)[r] = GetColItem(r, column, type).w;
            break;

        case IT_float:
            bufsize = rows * sizeof(float);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r)
                ((float*) buffer)[r] = GetColItem(r, column, type).f;
            break;

        case IT_double:
            bufsize = rows * sizeof(double);
            buffer = malloc(bufsize);
            for (r = 0; r < rows; ++r)
                ((double*) buffer)[r] = GetColItem(r, column, type).d;
            break;

        default: Assert(0); return 0;
    }

    /* start using 16-byte alignment once the emitted data reaches 1 Mb */
    /* only do this for vectors >= 128 bytes, worst-case waste is under 10% */
    
    Assert(!(bufsize > 0 && rows == 0));
    if (bufsize >= 128 && bufsize / rows >= 2) 
        EmitAlign(eip);
        
    EmitPair(eip, EmitBlock(eip, buffer, bufsize));
    
    return bufsize != 0;
}

static void EmitVarCol (EmitInfo_p eip, Column column, int istext) {
    int r, rows, bytes, *sizevec;
    Int_t buflen;
    Item item;
    struct Buffer buffer;
    Seq_p sizes;

    InitBuffer(&buffer);
    rows = column.seq->count;
    sizes = NewIntVec(rows, &sizevec);

    if (istext)
        for (r = 0; r < rows; ++r) {
            item = GetColItem(r, column, IT_string);
            bytes = strlen(item.s);
            if (bytes > 0)
                AddToBuffer(&buffer, item.s, ++bytes);
            sizevec[r] = bytes;
        }
    else
        for (r = 0; r < rows; ++r) {
            item = GetColItem(r, column, IT_bytes);
            AddToBuffer(&buffer, item.u.ptr, item.u.len);
            sizevec[r] = item.u.len;
        }

    buflen = BufferFill(&buffer);
    EmitPair(eip, EmitBuffer(eip, &buffer));    
    if (buflen > 0)
        EmitFixCol(eip, SeqAsCol(sizes), 1);

    EmitVarInt(eip, 0); /* no memos */
}

static void EmitSubCol (EmitInfo_p eip, Column column, int describe) {
    int r, rows;
    View_p view;
    struct Buffer newcolbuf;
    Buffer_p origcolbuf;

    origcolbuf = eip->colbuf;
    eip->colbuf = &newcolbuf;
    InitBuffer(eip->colbuf);

    rows = column.seq->count;

    for (r = 0; r < rows; ++r) {
        view = GetColItem(r, column, IT_view).v;
        EmitView(eip, view, describe);
    }

    eip->colbuf = origcolbuf;

    EmitPair(eip, EmitBuffer(eip, &newcolbuf));  
}

static void EmitCols (EmitInfo_p eip, View_p view, View_p maps) {
    int rows = ViewSize(view);
    
    EmitVarInt(eip, rows);

    if (rows > 0) {
        int i, r, c, *rowptr = NULL;
        ItemTypes type;
        Column column;
        Seq_p rowmap;
        View_p v, subv, meta = V_Meta(view);

        rowmap = NULL;
        
        for (c = 0; c < ViewWidth(view); ++c) {          
            if (maps != NULL) {
                Column mapcol = ViewCol(maps, c);
                if (!EmitFixCol(eip, mapcol, IT_int))
                    continue;

                if (rowmap == NULL)
                    rowmap = NewIntVec(rows, &rowptr);

                i = 0;
                for (r = 0; r < rows; ++r)
                    if (GetColItem(r, mapcol, IT_int).i)
                        rowptr[i++] = r;
                        
                rowmap->count = i;
                        
                v = RemapSubview(view, SeqAsCol(rowmap), 0, -1);
            } else
                v = view;
                
            column = ViewCol(v,c);
            type = ViewColType(view, c);
            switch (type) {
                
                case IT_int:
                case IT_wide:
                case IT_float:
                case IT_double:
                    EmitFixCol(eip, column, type); 
                    break;
                    
                case IT_string:
                    EmitVarCol(eip, column, 1);
                    break;
                    
                case IT_bytes:
                    EmitVarCol(eip, column, 0);
                    break;
                    
                case IT_view:
                    subv = GetViewItem(meta, c, MC_subv, IT_view).v;
                    EmitSubCol(eip, column, ViewSize(subv) == 0);
                    break;
                    
                default: Assert(0);
            }
        }
    }
}

static void EmitView (EmitInfo_p eip, View_p view, int describe) {
    EmitVarInt(eip, 0);

    if (eip->diff) {
        Seq_p mods = MutPrepare(view), *seqs = (void*) (mods + 1);

        EmitVarInt(eip, 0);
        EmitFixCol(eip, SeqAsCol(seqs[MP_delmap]), IT_int);

        if (EmitFixCol(eip, SeqAsCol(seqs[MP_adjmap]), IT_int))
            EmitCols(eip, seqs[MP_adjdat], seqs[MP_usemap]);

        EmitCols(eip, seqs[MP_insdat], NULL);
        if (ViewSize(seqs[MP_insdat]) > 0)
            EmitFixCol(eip, SeqAsCol(seqs[MP_insmap]), IT_int);
    } else {
        if (describe) {
            int cnt;
            char *ptr = NULL;
            struct Buffer desc;

            InitBuffer(&desc);
            MetaAsDesc(V_Meta(view), &desc);

            EmitVarInt(eip, BufferFill(&desc));
            
            while (NextBuffer(&desc, &ptr, &cnt))
                AddToBuffer(eip->colbuf, ptr, cnt);
        }

        EmitCols(eip, view, NULL);
    }
}

static void SetBigEndian32 (char* dest, Int_t value) {
    dest[0] = (char) (value >> 24);
    dest[1] = (char) (value >> 16);
    dest[2] = (char) (value >> 8);
    dest[3] = (char) value;
}

static Int_t EmitComplete (EmitInfo_p eip, View_p view) {
    int overflow;
    Int_t rootpos, tailpos, endpos;
    char tail[16];
    struct Head { short a; char b; char c; char d[4]; } head, *fixup;
    struct Buffer newcolbuf;

    if (eip->diff && !IsMutable(view))
        return 0;
        
    eip->position = 0;

    head.a = 'J' + ('L' << 8);
    head.b = 0x1A;
    head.c = 0;
    fixup = EmitCopy(eip, &head, sizeof head);

    eip->colbuf = &newcolbuf;
    InitBuffer(eip->colbuf);

    EmitView(eip, view, 1);

    EmitAlign(eip);
    rootpos = EmitBuffer(eip, eip->colbuf);  
    eip->colbuf = NULL;

    EmitAlign(eip);
    
    if (eip->position != (Int_t) eip->position)
        return -1; /* fail if >= 2 Gb on machines with a 32-bit address space */
    
    /* large files will have bit 8 of head[3] and bit 8 of tail[12] set */
    tailpos = eip->position;
    endpos = tailpos + sizeof tail;
    overflow = endpos >> 31;

    /* for file sizes > 2 Gb, store bits 41..31 in tail[2..3] */
    SetBigEndian32(tail, 0x80000000 + overflow);
    SetBigEndian32(tail+4, tailpos);
    SetBigEndian32(tail+8, 
                    ((eip->diff ? 0x90 : 0x80) << 24) + tailpos - rootpos);
    SetBigEndian32(tail+12, rootpos);
    if (overflow)
        tail[12] |= 0x80;
    
    EmitCopy(eip, tail, sizeof tail);
    
    if (overflow) {
        /* store bits 41..36 in head[3], and bits 35..4 in head[4..7] */
        Assert((endpos & 15) == 0);
        fixup->c = 0x80 | ((endpos >> 16) >> 20);
        SetBigEndian32(fixup->d, endpos >> 4);
    } else
        SetBigEndian32(fixup->d, endpos);
        
    return endpos;
}

Int_t ViewSave (View_p view, void *aux, SaveInitFun initfun, SaveDataFun datafun, int diff) {
    int i, numitems;
    Int_t bytes;
    struct Buffer buffer;
    EmitItem_p items;
    EmitInfo einfo;
    
    InitBuffer(&buffer);
    
    einfo.itembuf = &buffer;
    einfo.diff = diff;

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
