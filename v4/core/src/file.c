/*
 * file.c - Implementation of memory-mapped file access.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "intern.h"
#include "wrap_gen.h"

static View_p MapSubview (MappedFile_p, Int_t, View_p, View_p); /* forward */

#define MF_Data(map) ((const char*) ((map)->data[0].p))
#define MF_Length(map) ((Int_t) (map)->data[1].p)

static void MappedCleaner (MappedFile_p map) {
    Cleaner fun = *((Cleaner*) (map + 1));
    if (fun != NULL)
        fun(map);
}

static struct SeqType ST_Mapped = { "mapped", NULL, 0, MappedCleaner };

MappedFile_p InitMappedFile (const char *data, Int_t length, Cleaner fun) {
    MappedFile_p map;
    
    map = NewSequence(0, &ST_Mapped, sizeof(Cleaner));
    /* data[0] points to the current start of the mapped area */
    /* data[1] is the current length of the mapped area */
    /* data[2] points to the original mapped area */
    /* data[3] is available for storing an Object_p, see ext_tcl.c */
    map->data[0].p = (void*) data;
    map->data[1].p = (void*) length;
    map->data[2].p = (void*) data;
    
    *((Cleaner*) (map + 1)) = fun; /* TODO: get rid of this ugly hack */
    
    return map;
}

static void MappedFileCleaner (MappedFile_p map) {
#if WIN32+0
    UnmapViewOfFile(map->data[2].p);
#else
    int offset = MF_Data(map) - (const char*) map->data[2].p;
    munmap(map->data[2].p, MF_Length(map) + offset);
#endif
}

#if WIN32+0
/*
 * If we are opening a Windows PE executable with an attached metakit
 * then we must check for the presence of an Authenticode certificate
 * and reduce the length of our mapped region accordingly
 */

static DWORD
AuthenticodeOffset(LPBYTE pData, DWORD cbData)
{
    if (pData[0] == 'M' && pData[1] == 'Z')              /* IMAGE_DOS_SIGNATURE */
    {
        LPBYTE pNT = pData + *(LONG *)(pData + 0x3c);    /* e_lfanew */
        if (pNT[0] == 'P' && pNT[1] == 'E' && pNT[2] == 0 && pNT[3] == 0)
        {                                                /* IMAGE_NT_SIGNATURE */
            DWORD dwCheckSum = 0, dwDirectories = 0;
            LPBYTE pOpt = pNT + 0x18;                    /* OptionalHeader */
            LPDWORD pCertDir = NULL;
            if (pOpt[0] == 0x0b && pOpt[1] == 0x01) {    /* IMAGE_NT_OPTIONAL_HDR_MAGIC */
                dwCheckSum = *(DWORD *)(pOpt + 0x40);    /* Checksum */
                dwDirectories = *(DWORD *)(pOpt + 0x5c); /* NumberOfRvaAndSizes */
                if (dwDirectories > 4) {                 /* DataDirectory[] */
                    pCertDir = (DWORD *)(pOpt + 0x60 + (4 * 8));
                }
            } else {
                dwCheckSum = *(DWORD *)(pOpt + 0x40);    /* Checksum */
                dwDirectories = *(DWORD *)(pOpt + 0x6c); /* NumberOfRvaAndSizes */
                if (dwDirectories > 4) {                 /* DataDirectory[] */
                    pCertDir = (DWORD *)(pOpt + 0x70 + (4 * 8));
                }
            }

            if (pCertDir && pCertDir[1] > 0) {
                int n = 0;
                cbData = pCertDir[0];
                /* need to eliminate any zero padding - up to 8 bytes */
                while (pData[cbData - 16] != 0x80 && pData[cbData-1] == 0 && n < 16) {
                    --cbData, ++n;
                }
            }
        }
    }
    return cbData;
}
#endif /* WIN32 */

static MappedFile_p OpenMappedFile (const char *filename) {
    const char *data = NULL;
    Int_t length = -1;
        
#if WIN32+0
    {
        DWORD n;
        HANDLE h, f = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE_VALUE) {
            h = CreateFileMapping(f, 0, PAGE_READONLY, 0, 0, 0);
            if (h != INVALID_HANDLE_VALUE) {
                n = GetFileSize(f, 0);
                data = MapViewOfFile(h, FILE_MAP_READ, 0, 0, n);
                if (data != NULL) {
                    length = AuthenticodeOffset((LPBYTE)data, n);
                }
                CloseHandle(h);
            }
            CloseHandle(f);
        }
    }
#else
    {
        struct stat sb;
        int fd = open(filename, O_RDONLY);
        if (fd != -1) {
            if (fstat(fd, &sb) == 0) {
                data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
                if (data != MAP_FAILED)
                    length = sb.st_size;
            }
            close(fd);
        }
    }
#endif

    if (length < 0)
        return NULL;
        
    return InitMappedFile(data, length, MappedFileCleaner);
}

static void AdjustMappedFile (MappedFile_p map, int offset) {
    map->data[0].p = (void*) (MF_Data(map) + offset);
    map->data[1].p = (void*) (MF_Length(map) - offset);
}

static int IsReversedEndian(MappedFile_p map) {
#if _BIG_ENDIAN+0
    return *MF_Data(map) == 'J';
#else
    return *MF_Data(map) == 'L';
#endif
}

static Int_t GetVarInt (const char **cp) {
    int8_t b;
    Int_t v = 0;
    do {
        b = *(*cp)++;
        v = (v << 7) + b;
    } while (b >= 0);
    return v + 128;
}

static Int_t GetVarPair(const char** cp) {
    Int_t n = GetVarInt(cp);
    if (n > 0 && GetVarInt(cp) == 0)
        *cp += n;
    return n;
}

#define MM_cache        data[0].p
#define MM_offvec       data[1].q
#define MM_mapf         data[2].q
#define MM_meta         data[3].q

#define MM_offsets  MM_offvec->data[0].p
#define MM_base         MM_offvec->data[1].q


static void MappedViewCleaner (Seq_p seq) {
    int i, count;
    const View_p *subviews = seq->MM_cache;
    
    count = seq->MM_offvec->count;
    for (i = 0; i < count; ++i)
        DecRefCount(subviews[i]);
    
    DecRefCount(seq->MM_base);
}

static ItemTypes MappedViewGetter (int row, Item_p item) {
    Seq_p seq = item->c.seq;
    View_p *subviews = seq->MM_cache;
    
    if (subviews[row] == NULL) {
        Seq_p base = seq->MM_base;
        const Int_t *offsets = seq->MM_offsets;
        
        if (base != NULL)
            base = GetViewItem(base, row, item->c.pos, IT_view).v;
        
        subviews[row] = IncRefCount(MapSubview(seq->MM_mapf, offsets[row],
                                                        seq->MM_meta, base));
    }
    
    item->v = subviews[row];
    return IT_view;
}

static struct SeqType ST_MappedView = {
    "mappedview", MappedViewGetter, 01110, MappedViewCleaner
};

static Seq_p MappedViewCol (MappedFile_p map, int rows, const char **nextp, View_p meta, View_p base) {
    int r, c, cols, subcols;
    Int_t colsize, colpos, *offsets;
    const char *next;
    Seq_p offseq, result;
    
    offseq = NewPtrVec(rows, &offsets);
    
    cols = ViewSize(meta);
    
    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;
    next = MF_Data(map) + colpos;
    
    for (r = 0; r < rows; ++r) {
        offsets[r] = next - MF_Data(map);
        GetVarInt(&next);
        if (cols == 0) {
            Int_t desclen = GetVarInt(&next);
            const char *desc = next;
            next += desclen;
            meta = DescAsMeta(&desc, next);
        }
        if (GetVarInt(&next) > 0) {
            subcols = ViewSize(meta);
            for (c = 0; c < subcols; ++c)
                switch (GetColType(meta, c)) {
                    case 'B': case 'S': if (GetVarPair(&next))
                                            GetVarPair(&next);
                                        /* fall through */
                    default:            GetVarPair(&next);
                }
        }
    }
    
    result = NewSequence(rows, &ST_MappedView, rows * sizeof(View_p));
    /* data[0] points to a subview cache */
    /* data[1] points to a sequence owning the offest vector */
    /* data[2] points to the mapped file */
    /* data[3] points to the meta view */
    result->MM_offvec = IncRefCount(offseq);
    result->MM_mapf = IncRefCount(map);
    result->MM_meta = IncRefCount(cols > 0 ? meta : EmptyMetaView());
    /* offseq->data[1] points to the base view if there is one */
    result->MM_base = IncRefCount(base);
    
    /* TODO: could combine subview cache and offsets vector */
    
    return result;
}

static struct SeqType ST_MappedFix = { "mappedfix", NULL, 010 };

static Seq_p MappedFixedCol (MappedFile_p map, int rows, const char **nextp, int isreal) {
    Int_t colsize, colpos;
    const char *data;
    Seq_p result;

    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;
    data = MF_Data(map) + colpos;
    
    /* if bit count is too low-for single-bit vectors then it's compressed */
    if (rows >= 128 && 0 < colsize && colsize < rows/8) {
        int from, pos = 0;
        
        result = NewBitVec(rows);
        from = *data & 0x80 ? 0 : NextElias(data, colsize, &pos);
            
        for (;;) {
            int count = NextElias(data, colsize, &pos);
            if (count == 0)
                break;
            SetBitRange(result, from, count);
            from += count + NextElias(data, colsize, &pos);
        }
    } else {    
        int rev = IsReversedEndian(map);
    
        result = NewSequence(rows, &ST_MappedFix, 0);
        result->getter = FixedGetter(colsize, rows, isreal, rev)->getfun;
        /* data[0] points to the mapped data */
        /* data[1] points to the mapped file */
        result->data[0].p = (void*) data;
        result->data[1].p = IncRefCount(map);
    }
    
    return result;
}

#define MS_offvec       data[0].q
#define MS_offptr       data[1].p
#define MS_mapf         data[2].q
#define MS_sizes        data[3].q

static ItemTypes MappedStringGetter (int row, Item_p item) {
    const Int_t *offsets = item->c.seq->MS_offptr;
    const char *data = MF_Data(item->c.seq->MS_mapf);

    if (offsets[row] == 0)
        item->s = "";
    else if (offsets[row] > 0)
        item->s = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        if (GetVarInt(&next) > 0)
            item->s = data + GetVarInt(&next);
        else
            item->s = "";
    }

    return IT_string;
}

static struct SeqType ST_MappedString = {
    "mappedstring", MappedStringGetter, 01101
};

static ItemTypes MappedBytesGetter (int row, Item_p item) {
    Seq_p seq = item->c.seq;
    const Int_t *offsets = seq->MS_offptr;
    const char *data = MF_Data(seq->MS_mapf);
    
    item->u.len = GetSeqItem(row, seq->MS_sizes, IT_int).i;
    
    if (offsets[row] >= 0)
        item->u.ptr = (const Byte_t*) data + offsets[row];
    else {
        const char *next = data - offsets[row];
        item->u.len = GetVarInt(&next);
        item->u.ptr = (const Byte_t*) data + GetVarInt(&next);
    }

    return IT_bytes;
}

static struct SeqType ST_MappedBytes = {
    "mappedstring", MappedBytesGetter, 01101
};

static Seq_p MappedStringCol (MappedFile_p map, int rows, const char **nextp, int istext) {
    int r, len, memopos;
    Int_t colsize, colpos, *offsets;
    const char *next, *limit;
    Seq_p offseq, result, sizes;

    offseq = NewPtrVec(rows, &offsets);

    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;

    if (colsize > 0) {
        sizes = MappedFixedCol(map, rows, nextp, 0);
        for (r = 0; r < rows; ++r) {
            len = GetSeqItem(r, sizes, IT_int).i;
            if (len > 0) {
                offsets[r] = colpos;
                colpos += len;
            }
        }
    } else
        sizes = NewSequence(rows, FixedGetter(0, rows, 0, 0), 0);

    colsize = GetVarInt(nextp);
    memopos = colsize > 0 ? GetVarInt(nextp) : 0;
    next = MF_Data(map) + memopos;
    limit = next + colsize;
    
    /* negated offsets point to the size/pos pair in the map */
    for (r = 0; next < limit; ++r) {
        r += (int) GetVarInt(&next);
        offsets[r] = MF_Data(map) - next; /* always < 0 */
        GetVarPair(&next);
    }

    result = NewSequence(rows, istext ? &ST_MappedString : &ST_MappedBytes, 0);
    /* data[0] points to a sequence owning the offset vector */
    /* data[1] points to that vector of subview offsets */
    /* data[2] points to the mapped file */
    /* data[3] points to sizes seq if binary or is null if zero-terminated */
    result->MS_offvec = IncRefCount(offseq);
    result->MS_offptr = offsets;
    result->MS_mapf = IncRefCount(map);
    result->MS_sizes = istext ? NULL : IncRefCount(sizes);

    return result;
}

static Seq_p MappedBits (MappedFile_p map, int rows, const char **nextp) {
    Seq_p seq;
    
    if ((**nextp & 0xFF) == 0x80) {
        ++*nextp;
        return NULL; /* zero-sized column */
    }
        
    seq = MappedFixedCol(map, rows, nextp, 0);

    /* if this does not use 1-bit encoding, then we need to convert back */
    if (rows < 8 && seq->getter != PickIntGetter(1)->getfun) {
        int i;
        Seq_p result = NULL;

        for (i = 0; i < rows; ++i)
            if (GetSeqItem(i, seq, IT_int).i)
                SetBit(&result, i);
                
        return result;
    }
    
    return seq;
}

static View_p MapCols (MappedFile_p map, const char **nextp, View_p meta, View_p base, Seq_p adjseq) {
    int i, c, r, rows, *rowptr = NULL;
    View_p result, subview;
    Seq_p seq, usedmap = NULL, rowmap;
    Item item;
    
    rows = (int) GetVarInt(nextp);
    
    if (ViewSize(meta) == 0)
        return NoColumnView(rows);
    
    if (base != NULL) {
        int pos = 0, from = 0, count = 0;
        
        rowmap = NewIntVec(rows, &rowptr);
        while (NextBits(adjseq, &from, &count))
            for (i = 0; i < count; ++i)
                rowptr[pos++] = from + i;
                
        result = base;
    } else
        result = NewView(meta);
    
    if (rows > 0)
        for (c = 0; c < ViewWidth(result); ++c) {
            if (base != NULL) {
                usedmap = MappedBits(map, rows, nextp);
                r = CountBits(usedmap);
            } else
                r = rows;
                
            switch (ViewColType(result, c)) {
                
                case IT_int:
                case IT_wide:
                    seq = MappedFixedCol(map, r, nextp, 0); 
                    break;

                case IT_float:
                case IT_double:
                    seq = MappedFixedCol(map, r, nextp, 1);
                    break;
                    
                case IT_string:
                    seq = MappedStringCol(map, r, nextp, 1); 
                    break;
                    
                case IT_bytes:
                    seq = MappedStringCol(map, r, nextp, 0);
                    break;
                    
                case IT_view:
                    subview = GetViewItem(meta, c, MC_subv, IT_view).v;
                    seq = MappedViewCol(map, r, nextp, subview, base); 
                    break;
                    
                default:
                    Assert(0);
                    return result;
            }
            
            if (base != NULL) {
                i = 0;
                for (r = 0; r < usedmap->count; ++r)
                    if (TestBit(usedmap, r)) {
                        item.c.seq = seq;
                        item.c.pos = -1;
                        GetItem(i++, &item);
                        result = ViewSet(result, rowptr[r], c, &item);
                    }
                Assert(i == seq->count);
            } else
                SetViewSeqs(result, c, 1, seq);
        }

    return result;
}

static View_p MapSubview (MappedFile_p map, Int_t offset, View_p meta, View_p base) {
    const char *next;
    
    next = MF_Data(map) + offset;
    GetVarInt(&next);
    
    if (base != NULL) {
        int inscnt;
        View_p insview;
        Seq_p delseq, insseq, adjseq;
        
        meta = V_Meta(base);
        
        GetVarInt(&next); /* structure changes and defaults, NOTYET */

        delseq = MappedBits(map, ViewSize(base), &next);
        if (delseq != NULL) {
            int delcnt = 0, from = 0, count = 0;
            while (NextBits(delseq, &from, &count)) {
                base = ViewReplace(base, from - delcnt, count, NULL);
                delcnt += count;
            }
        }
            
        adjseq = MappedBits(map, ViewSize(base), &next);
        if (adjseq != NULL)
            base = MapCols(map, &next, meta, base, adjseq);

        insview = MapCols(map, &next, meta, NULL, NULL);
        inscnt = ViewSize(insview);
        if (inscnt > 0) {
            int shift = 0, from = 0, count = 0;
            insseq = MappedBits(map, ViewSize(base) + inscnt, &next);
            while (NextBits(insseq, &from, &count)) {
                View_p newdata = StepView(insview, count, shift, 1, 1);
                base = ViewReplace(base, from, 0, newdata);
                shift += count;
            }
        }
    
        return base;
    }
    
    if (ViewSize(meta) == 0) {
        Int_t desclen = GetVarInt(&next);
        const char *desc = next;
        next += desclen;
        meta = DescAsMeta(&desc, next);
    }

    return MapCols(map, &next, meta, NULL, NULL);
}

static int BigEndianInt32 (const char *p) {
    const Byte_t *up = (const Byte_t*) p;
    return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

View_p MappedToView (MappedFile_p map, View_p base) {
    int i, t[4];
    Int_t datalen, rootoff;
    
    if (MF_Length(map) <= 24 || *(MF_Data(map) + MF_Length(map) - 16) != '\x80')
        return NULL;
        
    for (i = 0; i < 4; ++i)
        t[i] = BigEndianInt32(MF_Data(map) + MF_Length(map) - 16 + i * 4);
        
    datalen = t[1] + 16;
    rootoff = t[3];

    if (rootoff < 0) {
        const Int_t mask = 0x7FFFFFFF; 
        datalen = (datalen & mask) + ((Int_t) ((t[0] & 0x7FF) << 16) << 15);
        rootoff = (rootoff & mask) + (datalen & ~mask);
        /* FIXME: rollover at 2 Gb, prob needs: if (rootoff > datalen) ... */
    }
    
    AdjustMappedFile(map, MF_Length(map) - datalen);
    return MapSubview(map, rootoff, EmptyMetaView(), base);
}

View_p OpenDataFile (const char *filename) {
    MappedFile_p map;
    
    map = OpenMappedFile(filename);
    if (map == NULL)
        return NULL;
        
    return MappedToView(map, NULL);
}
