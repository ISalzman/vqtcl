/*  Implementation of bitwise operations.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

int TestBit (Seq_p bitmap, int row) {
    if (bitmap != NULL && row < bitmap->count) {
        const Byte_t *bits = bitmap->data[0].p;
        return (bits[row>>3] >> (row&7)) & 1;
    }
    return 0;
}

int SetBit (Seq_p *pbitmap, int row) {
    Byte_t *bits;
    Seq_p bitmap = *pbitmap;
    
    if (bitmap == NULL)
        *pbitmap = bitmap = IncRefCount(NewBitVec(row+1));

    if (TestBit(bitmap, row))
        return 0;
    
    if (row >= bitmap->count) {
        int bytes = bitmap->data[1].i;
        if (row >= 8 * bytes) {
            Seq_p newbitmap = IncRefCount(NewBitVec(12 * bytes));
            memcpy(newbitmap->data[0].p, LoseRef(bitmap)->data[0].p, bytes);
            *pbitmap = bitmap = newbitmap;
        }
        bitmap->count = row + 1;
    }

    bits = bitmap->data[0].p;
    bits[row>>3] |= 1 << (row&7);
    
    return 8 * bitmap->data[1].i;
}

void ClearBit (Seq_p bitmap, int row) {
    if (TestBit(bitmap, row)) {
        Byte_t *bits = bitmap->data[0].p;
        bits[row>>3] &= ~ (1 << (row&7));
    }
}

void SetBitRange (Seq_p bits, int from, int count) {
    while (--count >= 0)
        SetBit(&bits, from++);
}

Seq_p MinBitCount (Seq_p *pbitmap, int count) {
    if (*pbitmap == NULL || (*pbitmap)->count < count) {
        SetBit(pbitmap, count);
        ClearBit(*pbitmap, count);
        --(*pbitmap)->count;
    }
    return *pbitmap;
}

int NextBits (Seq_p bits, int *fromp, int *countp) {
    int curr = *fromp + *countp;
    
    while (curr < bits->count && !TestBit(bits, curr))
        ++curr;
        
    *fromp = curr;
    
    while (curr < bits->count && TestBit(bits, curr))
        ++curr;
        
    *countp = curr - *fromp;
    return *countp;
}

int TopBit (int v) {
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

static void EmitBits(char *bytes, int val, int *ppos) {
    char *curr = bytes + (*ppos >> 3);
    int bit = 8 - (*ppos & 7);
    int top = TopBit(val), n = top + 1;
    
    Assert(val > 0);
    
    while (top >= bit) {
        ++curr;
        top -= bit;
        bit = 8;
    }
    
    bit -= top;
    
    while (n >= bit) {
        *curr++ |= (char) (val >> (n-bit));
        val &= (1 << (n-bit)) - 1;
        n -= bit;
        bit = 8;
    }
    
    *curr |= (char) (val << (bit - n));
    *ppos = ((curr - bytes) << 3) + (8 - bit) + n;
}

char *Bits2elias (const char *bytes, int count, int *outbits) {
    int i, last, bits = 0;
    char *out;
    
    if (count <= 0) {
        *outbits = 0;
        return NULL;
    }
    
    out = calloc(1, (count+count/2)/8+1);
    last = *bytes & 1;
    *out = last << 7;
    *outbits = 1;
    for (i = 0; i < count; ++i) {
        if (((bytes[i>>3] >> (i&7)) & 1) == last)
            ++bits;
        else {
            EmitBits(out, bits, outbits);
            bits = 1;
            last ^= 1;
        }
    }

    if (bits)
        EmitBits(out, bits, outbits);
    
    return out;
}

int NextElias (const char *bytes, int count, int *inbits) {
    int val = 0;
    const char *in;
    
    if (*inbits == 0)
        *inbits = 1;
    
    in = bytes + (*inbits >> 3);
    if (in < bytes + count) {
        int i = 0, bit = 8 - (*inbits & 7);
        
        for (;;) {
            ++i;
            if (*in & (1 << (bit-1)))
                break;
            if (--bit <= 0) {
                if (++in >= bytes + count)
                    return 0;
                bit = 8;
            }
        }
        
        while (i >= bit) {
            val <<= bit;
            val |= (*in++ & ((1 << bit) - 1));
            i -= bit;
            bit = 8;
        }
        
        val <<= i;
        val |= (*in >> (bit - i)) & ((1 << i) - 1);
        *inbits = ((in - bytes) << 3) + (8 - bit) + i;
    }
    
    return val;
}

int CountBits (Seq_p seq) {
    int result = 0, from = 0, count = 0;

    if (seq != NULL)
        while (NextBits(seq, &from, &count))
            result += count;

    return result;
}

ItemTypes BitRunsCmd_i (Item_p a) {
    int i, outsize, count = a->c.seq->count, pos = 0;
    char *data;
    struct Buffer buffer;
    Seq_p temp;

    temp = NewBitVec(count);
    
    for (i = 0; i < count; ++i) {
        if (GetColItem(i, a->c, IT_int).i)
            SetBit(&temp, i);
    }

    data = Bits2elias(temp->data[0].p, count, &outsize);

    count = (outsize + 7) / 8;
    InitBuffer(&buffer);
    
    if (count > 0) {
        ADD_INT_TO_BUF(buffer, (*data & 0x80) != 0);
        while ((i = NextElias(data, count, &pos)) != 0)
            ADD_INT_TO_BUF(buffer, i);
    }
    
    free(data);
    
    a->c = SeqAsCol(BufferAsIntVec(&buffer));
    return IT_column;
}
