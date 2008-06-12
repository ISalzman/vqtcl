/*  Load views from a Metakit-format memory map.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#define MappedCol(p)  (vCast(p,struct vqMappedCol_s)-1)

typedef struct vqMappedCol_s {
    const char *data;
    vqView meta;        /* TODO: overlap meta & sizes, never used same time */
    vqColumn sizes;
    vqView map2;        /* TODO: could run up chain via owner */
                        /* but it's used a lot in S/B since they use offsets */
                        /* idea: maybe map2 could overlap data ptr for S/B ? */
    struct vqInfo_s _info;
} *vqMappedCol;

#if 0
union vqMappedCol_u {
    struct vqInfo_s info[1]; /* common header, accessed as info[-1] */
    struct vqMappedCol_s dv[1]; /* map-view specific, accessed as vhead[-1] */
};
#endif

/* forward */
static vqView MapSubview (vqView map, const char *base, vqView meta);

static vqType Rgetter_i0 (int row, vqSlot *cp) {
    VQ_UNUSED(row);
    cp->i = 0;
    return VQ_int;
}
static vqType Rgetter_i1 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data;
    cp->i = (ptr[row>>3] >> (row&7)) & 1;
    return VQ_int;
}
static vqType Rgetter_i2 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data;
    cp->i = (ptr[row>>2] >> 2*(row&3)) & 3;
    return VQ_int;
}
static vqType Rgetter_i4 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data;
    cp->i = (ptr[row>>1] >> 4*(row&1)) & 15;
    return VQ_int;
}
static vqType Rgetter_i8 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data;
    cp->i = (int8_t) ptr[row];
    return VQ_int;
}

#ifdef VQ_MUSTALIGN
static vqType Rgetter_i16 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data + row * 2;
#ifdef VQ_BIGENDIAN
    cp->i = (ptr[0] << 8) | ((const uint8_t*) ptr)[1];
#else
    cp->i = (ptr[1] << 8) | ((const uint8_t*) ptr)[0];
#endif
    return VQ_int;
}
static vqType Rgetter_i32 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        ((char*) cp)[i] = ptr[i];
    return VQ_int;
}
static vqType Rgetter_i64 (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        ((char*) cp)[i] = ptr[i];
    return VQ_long;
}
static vqType Rgetter_f32 (int row, vqSlot *cp) {
    Rgetter_i32(row, cp);
    return VQ_float;
}
static vqType Rgetter_f64 (int row, vqSlot *cp) {
    Rgetter_i64(row, cp);
    return VQ_double;
}
#else
static vqType Rgetter_i16 (int row, vqSlot *cp) {
    const short *ptr = (const void*) MappedCol(cp->c)->data;
    cp->i = ptr[row];
    return VQ_int;
}
static vqType Rgetter_i32 (int row, vqSlot *cp) {
    const int *ptr = (const void*) MappedCol(cp->c)->data;
    cp->i = ptr[row];
    return VQ_int;
}
static vqType Rgetter_i64 (int row, vqSlot *cp) {
    const int64_t *ptr = (const void*) MappedCol(cp->c)->data;
    cp->l = ptr[row];
    return VQ_long;
}
static vqType Rgetter_f32 (int row, vqSlot *cp) {
    const float *ptr = (const void*) MappedCol(cp->c)->data;
    cp->f = ptr[row];
    return VQ_float;
}
static vqType Rgetter_f64 (int row, vqSlot *cp) {
    const double *ptr = (const void*) MappedCol(cp->c)->data;
    cp->d = ptr[row];
    return VQ_double;
}
#endif

static vqType Rgetter_i16r (int row, vqSlot *cp) {
    const uint8_t *ptr = (const uint8_t*) MappedCol(cp->c)->data + row * 2;
#ifdef VQ_BIGENDIAN
    cp->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#else
    cp->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#endif
    return VQ_int;
}
static vqType Rgetter_i32r (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        ((char*) cp)[i] = ptr[3-i];
    return VQ_int;
}
static vqType Rgetter_i64r (int row, vqSlot *cp) {
    const char *ptr = MappedCol(cp->c)->data + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        ((char*) cp)[i] = ptr[7-i];
    return VQ_long;
}
static vqType Rgetter_f32r (int row, vqSlot *cp) {
    Rgetter_i32r(row, cp);
    return VQ_float;
}
static vqType Rgetter_f64r (int row, vqSlot *cp) {
    Rgetter_i64r(row, cp);
    return VQ_double;
}

static void Rcleaner (vqColumn cp) {
    vq_decref(MappedCol(cp)->map2);
}

static vqDispatch vget_i0   = {
    "get_i0"  , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i0  
};
static vqDispatch vget_i1   = {
    "get_i1"  , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i1  
};
static vqDispatch vget_i2   = {
    "get_i2"  , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i2  
};
static vqDispatch vget_i4   = {
    "get_i4"  , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i4  
};
static vqDispatch vget_i8   = {
    "get_i8"  , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i8  
};
static vqDispatch vget_i16  = {
    "get_i16" , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i16 
};
static vqDispatch vget_i32  = {
    "get_i32" , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i32 
};
static vqDispatch vget_i64  = {
    "get_i64" , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i64 
};
static vqDispatch vget_i16r = {
    "get_i16r", sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i16r
};
static vqDispatch vget_i32r = {
    "get_i32r", sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i32r
};
static vqDispatch vget_i64r = {
    "get_i64r", sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_i64r
};
static vqDispatch vget_f32  = {
    "get_f32" , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_f32 
};
static vqDispatch vget_f64  = {
    "get_f64" , sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_f64 
};
static vqDispatch vget_f32r = {
    "get_f32r", sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_f32r
};
static vqDispatch vget_f64r = {
    "get_f64r", sizeof(struct vqMappedCol_s), 0, Rcleaner, Rgetter_f64r
};

static vqDispatch* PickIntGetter (int bits) {
    switch (bits) {
        case 0:     return &vget_i0;
        case 1:     return &vget_i1;
        case 2:     return &vget_i2;
        case 4:     return &vget_i4;
        case 8:     return &vget_i8;
        case 16:    return &vget_i16;
        case 32:    return &vget_i32;
        case 64:    return &vget_i64;
    }
    assert(0);
    return 0;
}

static vqDispatch* FixedGetter (int bytes, int rows, int real, int flip) {
    static char widths[8][7] = {
        {0,-1,-1,-1,-1,-1,-1},
        {0, 8,16, 1,32, 2, 4},
        {0, 4, 8, 1,16, 2,-1},
        {0, 2, 4, 8, 1,-1,16},
        {0, 2, 4,-1, 8, 1,-1},
        {0, 1, 2, 4,-1, 8,-1},
        {0, 1, 2, 4,-1,-1, 8},
        {0, 1, 2,-1, 4,-1,-1},
    };
    int bits = rows < 8 && bytes < 7 ? widths[rows][bytes] : (bytes<<3) / rows;
    switch (bits) {
        case 16:    return flip ? &vget_i16r : &vget_i16;
        case 32:    return real ? flip ? &vget_f32r : &vget_f32
                                : flip ? &vget_i32r : &vget_i32;
        case 64:    return real ? flip ? &vget_f64r : &vget_f64
                                : flip ? &vget_i64r : &vget_i64;
    }
    return PickIntGetter(bits);
}

static int IsRevEndian (vqView map) {
#ifdef VQ_BIGENDIAN
    return *MappedFile(map)->data == 'J';
#else
    return *MappedFile(map)->data == 'L';
#endif
}

static intptr_t GetVarInt (const char **nextp) {
    int8_t b;
    intptr_t v = 0;
    do {
        b = *(*nextp)++;
        v = (v << 7) + b;
    } while (b >= 0);
    return v + 128;
}

static intptr_t GetVarPair (const char **nextp) {
    intptr_t n = GetVarInt(nextp);
    if (n > 0 && GetVarInt(nextp) == 0)
        *nextp += n;
    return n;
}

static vqView GetMetaDesc (vqView map, const char **nextp) {
    intptr_t desclen = GetVarInt(nextp);
    vqView meta = desc2meta(vwEnv(map), *nextp, desclen);
    *nextp += desclen;
    return meta;
}

static void MappedViewCleaner (vqColumn cp) {
    vqMappedCol dv = MappedCol(cp);
    vq_decref(dv->meta);
    vq_decref(dv->map2);
}

static vqType MappedViewGetter (int row, vqSlot *cp) {
    vqMappedCol dv = MappedCol(cp->c);
    /* can't use meta in parent if this view has its own structure, e.g. top */
    cp->v = MapSubview(dv->map2, cp->c->strings[row], dv->meta);
    return VQ_view;
}

static vqDispatch mvtab = {
    "mappedview", sizeof(struct vqMappedCol_s), 0,
    MappedViewCleaner, MappedViewGetter
};

static vqMappedCol MappedViewCol (vqView map, int rows, const char **nextp, vqView meta) {
    int r, c;
    const char **bases = alloc_vec(&mvtab, rows * sizeof *bases);
    vqMappedCol dv = MappedCol(bases);
    const char *next = 0;
    if (GetVarInt(nextp) > 0)
        next = MappedFile(map)->data + GetVarInt(nextp);
        
    for (r = 0; r < rows; ++r) {
        bases[r] = next;
        GetVarInt(&next);
        if (vSize(meta) == 0)
            meta = GetMetaDesc(map, &next);
        if (GetVarInt(&next) > 0)
            for (c = 0; c < vSize(meta); ++c) {
                switch (vq_getInt(meta, c, 1, VQ_nil) & VQ_TYPEMASK) {
                    case VQ_bytes: case VQ_string:
                        if (GetVarPair(&next))
                            GetVarPair(&next);
                }
                GetVarPair(&next);
            }
    }
    
    vSize((vqColumn) bases) = rows;
    dv->map2 = vq_incref(map);
    dv->meta = vq_incref(meta);    
    return (vqMappedCol) bases;
}

static vqMappedCol MappedFixedCol (vqView map, int rows, const char **nextp, int real) {
    intptr_t bytes = GetVarInt(nextp);
    void *p = alloc_vec(FixedGetter(bytes, rows, real, IsRevEndian(map)), 0);    
    vSize((vqColumn) p) = rows;
    MappedCol(p)->data = bytes > 0 ? MappedFile(map)->data + GetVarInt(nextp)
                                   : 0;
    MappedCol(p)->map2 = vq_incref(map);
    return p;
}

static void MappedStringCleaner (vqColumn cp) {
    vqMappedCol dv = MappedCol(cp);
    release_vec(dv->sizes);
    vq_decref(dv->map2);
}

static vqType MappedStringGetter (int row, vqSlot *cp) {
    vqMappedCol dv = MappedCol(cp->c);
    const intptr_t *offsets = cp->p;
    const char *data = MappedFile(dv->map2)->data;

    if (offsets[row] == 0)
        cp->s = "";
    else if (offsets[row] > 0)
        cp->s = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        GetVarInt(&next);
        cp->s = data + GetVarInt(&next);
    }

    return VQ_string;
}

static vqDispatch mstab = {
    "mappedstring", sizeof(struct vqMappedCol_s), 0,
    MappedStringCleaner, MappedStringGetter
};

static vqType MappedBytesGetter (int row, vqSlot *cp) {
    vqMappedCol dv = MappedCol(cp->c);
    const intptr_t *offsets = cp->p;
    const char *data = MappedFile(dv->map2)->data;
    
    if (offsets[row] == 0) {
        cp->x.y.i = 0;
        cp->p = 0;
    } else if (offsets[row] > 0) {
        cp->p = dv->sizes; /* reuse cell to access sizes */
        getcell(row, cp);
        cp->x.y.i = cp->i;
        cp->p = (char*) data + offsets[row];
    } else {
        const char *next = data - offsets[row];
        cp->x.y.i = GetVarInt(&next);
        cp->p = (char*) data + GetVarInt(&next);
    }

    return VQ_bytes;
}

static vqDispatch mbtab = {
    "mappedstring", sizeof(struct vqMappedCol_s), 0,
    MappedStringCleaner, MappedBytesGetter
};

static vqMappedCol MappedStringCol (vqView map, int rows, const char **nextp, int istext) {
    int r;
    intptr_t colsize, colpos, *offsets;
    const char *next, *limit;
    vqMappedCol result;
    vqColumn sizes;
    vqSlot cell;

    result = alloc_vec(istext ? &mstab : &mbtab, rows * sizeof(void*));
    offsets = (void*) result;

    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;

    if (colsize > 0) {
        sizes = (vqColumn) MappedFixedCol(map, rows, nextp, 0);
        for (r = 0; r < rows; ++r) {
            cell.p = sizes;
            getcell(r, &cell);
            if (cell.i > 0) {
                offsets[r] = colpos;
                colpos += cell.i;
            }
        }
    } else
        sizes = alloc_vec(FixedGetter(0, rows, 0, 0), 0);
        
    colsize = GetVarInt(nextp);
    next = MappedFile(map)->data + (colsize > 0 ? GetVarInt(nextp) : 0);
    limit = next + colsize;
    
    /* negated offsets point to the size/pos pair in the map */
    for (r = 0; next < limit; ++r) {
        r += (int) GetVarInt(&next);
        offsets[r] = MappedFile(map)->data - next;
        assert(offsets[r] < 0);
        GetVarPair(&next);
    }

    vSize((vqColumn) result) = rows;
    MappedCol(result)->map2 = vq_incref(map);
    
    if (istext)
        release_vec(sizes);
    else
        result[-1].sizes = sizes;

    return result;
}

static void SubViewCleaner (vqColumn cp) {
    vqSlot *aux = vwAuxP((vqView) cp);
    map_remember(aux->x.y.p, aux->s, 0);
    vq_decref(aux->x.y.p);
    ViewCleaner(cp);
}

static vqDispatch msvtab = {
    "subview", sizeof(struct vqView_s), 0, SubViewCleaner
};

static vqView MapSubview (vqView map, const char *next, vqView meta) {
    int c;
    vqView sub, v;
    const char *pos = next;
    vqSlot *aux;
    
    v = map_lookup(map, pos);
    if (v != 0)
        return v;
    
    GetVarInt(&next);
    if (vSize(meta) == 0)
        meta = GetMetaDesc(map, &next);

    v = IndirectView(&msvtab, meta, GetVarInt(&next), sizeof(vqSlot));
    aux = vwAuxP(v);
    aux->s = pos;
    aux->x.y.p = vq_incref(map);
    map_remember(map, pos, v);
    
    /* TODO: could splice here to do lazy setup on first use from here on */

    if (vSize(v) > 0)
        for (c = 0; c < vSize(meta); ++c) {
            vqMappedCol vec = 0;
            switch (ViewColType(v, c)) {
                case VQ_int:
                case VQ_long:
                    vec = MappedFixedCol(map, vSize(v), &next, 0); 
                    break;
                case VQ_float:
                case VQ_double:
                    vec = MappedFixedCol(map, vSize(v), &next, 1);
                    break;
                case VQ_string:
                    vec = MappedStringCol(map, vSize(v), &next, 1); 
                    break;
                case VQ_bytes:
                    vec = MappedStringCol(map, vSize(v), &next, 0);
                    break;
                case VQ_view:
                    sub = vq_getView(meta, c, 2, 0);
                    vec = MappedViewCol(map, vSize(v), &next, sub); 
                    break;
                default:
                    assert(0);
            }
            v->col[c].p = vec;
            own_datavec(v,c);
        }

    return v;
}

static int BigEndianInt32 (const char *p) {
    const uint8_t *up = (const uint8_t*) p;
    return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

vqView LoadView (vqView v) {
    int i, t[4];
    size_t limit, start;
    vqMap map = (void*) v;
    
    if (map->size <= 24 || *(map->data + map->size - 16) != '\x80')
        return 0;
        
    for (i = 0; i < 4; ++i)
        t[i] = BigEndianInt32(map->data + map->size - 16 + i * 4);
        
    limit = t[1] + 16;
    start = t[3];

    if ((intptr_t) start < 0) {
        const intptr_t mask = 0x7FFFFFFF; 
        limit = (limit & mask) + ((intptr_t) ((t[0] & 0x7FF) << 16) << 15);
        start = (start & mask) + (limit & ~mask);
        /* FIXME: rollover at 2 Gb, prob needs: if (start > limit) ... */
    }
    
    if (limit != map->size)
        v = new_map(vwEnv(v), 0, v, MappedFile(map)->data +
                                        MappedFile(map)->size - limit, limit);
    return MapSubview(v, MappedFile(v)->data + start, empty_meta(vwEnv(v)));
}
