/*  Core of the Vlerq engine code.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

/* forward */
static vqDispatch tvtab;
static vqView check_view (vqEnv env, int t);
static vqView table2view (vqEnv env, int t);
    
static void *alloc_vec (const vqDispatch *vtab, int bytes) {
    int hb = vtab->headerbytes;
    vqColumn result = (void*) ((char*) calloc(1, bytes + hb) + hb);
    vInfo(result).dispatch = vtab;
    return result;
}
static void release_vec (vqColumn v) {
    if (v != 0) {
        const vqDispatch *dp = vInfo(v).dispatch;
        if (dp->cleaner != 0)
            dp->cleaner(v);
        free((char*) v - dp->headerbytes);
    }
}
vqView vq_incref (vqView v) {
    if (v != 0)
        ++vwRefs(v); /* TODO: check for overflow */
    return v;
}
void vq_decref (vqView v) {
    if (v != 0 && --vwRefs(v) <= 0) {
        /*assert(vwRefs(v) == 0);*/
        if (vInfo(v).dispatch->cleaner != 0)
            vInfo(v).dispatch->cleaner((vqColumn) v);
        free((char*) v - vInfo(v).dispatch->headerbytes);
    }
}
static vqColumn incColRef(vqColumn p) {
    if (p != 0) {
        vqView v = vInfo(p).owner.view;
        assert(v != 0);
        vq_incref(v);
    }
    return p;
}
static void decColRef(vqColumn p) {
    if (p != 0) {
#if NOTYET
        vqView v = vInfo(p).owner.view;
        /* FIXME: column cleanup isn't working right yet! */
        assert(v != 0);
        vq_decref(v);
#endif
    }
}

static void *tagged_udata (vqEnv env, size_t bytes, const char *tag) {
    void *p = lua_newuserdata(env, bytes);
    luaL_getmetatable(env, tag);
    lua_setmetatable(env, -2);
    return p;
}

static vqView empty_meta (vqEnv env) {
    vqView v;
    lua_getfield(env, LUA_REGISTRYINDEX, "vq.emv"); /* t */
    v = check_view(env, -1);
    lua_pop(env, 1);
    return v;
}

static int char2type (char c) {
    const char *p = strchr(VQ_TYPES, c & ~0x20);
    int type = p != 0 ? p - VQ_TYPES : VQ_nil;
    if (c & 0x20)
        type |= VQ_NULLABLE;
    return type;
}

static vqType string2type (const char *str) {
    int type = char2type(*str);
    while (*++str != 0)
        if ('a' <= *str && *str <= 'z')
        type |= 1 << (*str - 'a' + 5);
    return type;
}

static const char* type2string (vqType type, char *buf) {
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

static int push_view (vqView v) {
    vqEnv env = vwEnv(v);
    lua_getfield(env, LUA_REGISTRYINDEX, "vq.pool");           /* t */
    lua_pushlightuserdata(env, v);                             /* t p */
    lua_rawget(env, -2);                                       /* t u */
    /* create and store a new vq.view object if not found */
    if (lua_isnil(env, -1)) {
        vqView *ud = tagged_udata(env, sizeof *ud, "vq.view"); /* t n u */
        *ud = vq_incref(v);
        lua_pushlightuserdata(env, v);                         /* t n u p */
        lua_pushvalue(env, -2);                                /* t n u p u */
        lua_rawset(env, -5);                                   /* t n u */
        lua_remove(env, -2);                                   /* t u */
    }
    lua_remove(env, -2);                                       /* t */
    return 1;
}

static vqView desc_parser (vqView emv, char **desc, const char **nbuf, int *tbuf, vqView *sbuf) {
    char sep, *ptr = *desc;
    const char  **names = nbuf;
    int c, nc = 0, *types = tbuf;
    vqView result, *subvs = sbuf;
    
    for (;;) {
        const char* s = ptr;
        sbuf[nc] = emv;
        tbuf[nc] = VQ_string;
        
        while (strchr(":,[]", *ptr) == 0)
            ++ptr;
        sep = *ptr;
        *ptr = 0;

        if (sep == '[') {
            ++ptr;
            sbuf[nc] = desc_parser(emv, &ptr, nbuf+nc, tbuf+nc, sbuf+nc);
            tbuf[nc] = VQ_view;
            sep = *++ptr;
        } else if (sep == ':') {
            tbuf[nc] = char2type(*++ptr);
            sep = *++ptr;
        }
        
        nbuf[nc++] = s;
        if (sep != ',')
            break;
            
        ++ptr;
    }
    
    *desc = ptr;

    result = vq_new(vwMeta(emv), nc);
    for (c = 0; c < nc; ++c)
        vq_setMetaRow(result, c, names[c], types[c], subvs[c]);
    return result;
}
static vqView desc2meta (vqEnv env, const char *desc, int length) {
    int i, bytes, limit = 1;
    void *buffer;
    const char **nbuf;
    int *tbuf;
    vqView *sbuf, meta;
    char *dbuf;
    vqView emv = empty_meta(env);
    
    if (length == 0)
        return emv;

    if (length < 0)
        length = strlen(desc);
    
    /* find a hard upper bound for the buffer requirements */
    for (i = 0; i < length; ++i)
        if (desc[i] == ',' || desc[i] == '[')
            ++limit;
    
    bytes = limit * (2 * sizeof(void*) + sizeof(int)) + length + 1;
    buffer = malloc(bytes);
    nbuf = buffer;
    tbuf = (void*) (nbuf + limit);
    sbuf = (void*) (tbuf + limit);
    dbuf = memcpy(sbuf + limit, desc, length);
    dbuf[length] = ']';
    
    meta = desc_parser(emv, &dbuf, nbuf, tbuf, sbuf);
    
    free(buffer);
    return meta;
}

static void view_attach (vqColumn cp, int row) {
    vqView v;
    assert(cp != 0);
    assert(vInfo(cp).dispatch == &tvtab);
    v = cp->views[row];
    if (v != 0) {
        vqColumn o = vInfo(v).owner.col;
        if (o == 0 || o == cp) {
            vInfo(v).owner.col = cp;
            vInfo(v).ownerpos = row;
        }
    }
}
static void view_detach (vqColumn cp, int row) {
    vqView v;
    assert(cp != 0);
    assert(vInfo(cp).dispatch == &tvtab);
    v = cp->views[row];
    if (v != 0 && vInfo(v).owner.col == cp)
        vInfo(v).owner.col = 0;
}

static void attach_range (vqColumn cp, int from, int cnt) {
    if (vInfo(cp).dispatch == &tvtab)
        while (--cnt >= 0)
            view_attach(cp, from++);
}
static void detach_range (vqColumn cp, int from, int cnt) {
    if (vInfo(cp).dispatch == &tvtab)
        while (--cnt >= 0)
            view_detach(cp, from++);
}

/* TODO: could pass vqColumn iso vqColumn*, since owner identifies parent */
void *VecInsert (vqColumn *vecp, int off, int cnt) {
    vqColumn v = *vecp, v2;
    int unit = vInfo(v).dispatch->itembytes, limit = vLimit(v), size = vSize(v),
        above = size - off, endins = off + cnt, newsize = size + cnt;
    char *cvec = (char*) v, *cvec2;
    assert(cnt > 0 && unit > 0 && above >= 0);
    /* TODO should always be the case: assert(vInfo(v).owner.view != 0); */
    if (vInfo(v).owner.view != 0)
        assert(vInfo(v).owner.view->col[vInfo(v).ownerpos].c == v);
    if (newsize <= limit) {
        detach_range(v, off, above);
        memmove(cvec + endins * unit, cvec + off * unit, above * unit);
        memset(cvec + off * unit, 0, cnt * unit);
        attach_range(v, endins, above);
    } else {
        limit += limit/2;
        if (limit < newsize)
            limit = newsize;
        v2 = alloc_vec(vInfo(v).dispatch, limit * unit);
        cvec2 = (char*) v2;
        vSize(v2) = size;
        vLimit(v2) = limit;
        detach_range(v, 0, size);
        memcpy(v2, v, off * unit);
        memcpy(cvec2 + endins * unit, cvec + off * unit, above * unit);
        attach_range(v2, 0, off);
        attach_range(v2, endins, above);
        /* column replacement must also adjust view owner.view/pos */
        vInfo(v2).owner.view = vInfo(v).owner.view;
        vInfo(v2).ownerpos = vInfo(v).ownerpos;
        vInfo(v).owner.view = 0; /* break original tie with owner view */
        vSize(v) = 0; /* prevent vq_decref's of moved entries */
        release_vec(v);
        *vecp = v = v2;
    }
    vSize(v) = newsize;
    return v;
}

void *VecDelete (vqColumn *vecp, int off, int cnt) {
    /* TODO: shrink when less than half full and > 10 elements */
    vqColumn v = *vecp;
    int unit = vInfo(v).dispatch->itembytes;
    char *cvec = (char*) v;
    assert(cnt > 0 && unit > 0 && off + cnt <= vSize(v));
    detach_range(v, off, cnt);
    memmove(cvec+off*unit, cvec+(off+cnt)*unit, (vSize(v)-(off+cnt))*unit);
    vSize(v) -= cnt;
    return v;
} 

static vqType getcell (int row, vqSlot *cp) {
    vqColumn v = cp->p;
    if (v == 0)
        return VQ_nil;
    if (row < 0)
        row += vSize(v);
    if (row < 0 || row >= vSize(v))
        return VQ_nil;
    assert(vInfo(v).dispatch->getter != 0);
    return vInfo(v).dispatch->getter(row, cp);
}

static void DataVecReplacer (vqView t, int off, int del, vqView data) {
    int c, r, cols = vwCols(t), ins = data != 0 ? vSize(data) : 0;
    assert(off >= 0 && del >= 0 && off + del <= vSize(t));
    vSize(t) += ins;
    for (c = 0; c < cols; ++c) {
        if (ins > del) /* TODO: could use separate ins/del code for views */
            VecInsert(&t->col[c].c, off, ins - del);
        else if (del > ins)
            VecDelete(&t->col[c].c, off, del - ins);
        if (data != 0 && c < vwCols(data))
            for (r = 0; r < ins; ++r) {
                vqSlot cell = data->col[c];
                vqType type = getcell(r, &cell);
                vq_set(t, off + r, c, type, cell);
            }
    }
    vSize(t) -= del;
}

static vqType NilVecGetter (int row, vqSlot *cp) {
    const char *p = cp->p;
    cp->i = (p[row/8] >> (row&7)) & 1;
    return cp->i ? VQ_int : VQ_nil;
}
static vqType IntVecGetter (int row, vqSlot *cp) {
    const int *p = cp->p;
    cp->i = p[row];
    return VQ_int;
}
static vqType PosVecGetter (int row, vqSlot *cp) {
    const int *p = cp->p;
    cp->i = p[row];
    return VQ_pos;
}
static vqType LongVecGetter (int row, vqSlot *cp) {
    const int64_t *p = cp->p;
    cp->l = p[row];
    return VQ_long;
}
static vqType FloatVecGetter (int row, vqSlot *cp) {
    const float *p = cp->p;
    cp->f = p[row];
    return VQ_float;
}
static vqType DoubleVecGetter (int row, vqSlot *cp) {
    const double *p = cp->p;
    cp->d = p[row];
    return VQ_double;
}
static vqType StringVecGetter (int row, vqSlot *cp) {
    const char **p = cp->p;
    cp->s = p[row] != 0 ? p[row] : "";
    return VQ_string;
}
static vqType BytesVecGetter (int row, vqSlot *cp) {
    vqSlot *p = cp->p;
    *cp = p[row];
    return VQ_bytes;
}
static vqType ViewVecGetter (int row, vqSlot *cp) {
    vqView *p = cp->p;
    cp->v = p[row];
    return VQ_view;
}

static void NilVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    int bit = 1 << (row&7);
    VQ_UNUSED(col);
    if (cp != 0)
        q->chars[row/8] |= bit;
    else
        q->chars[row/8] &= ~bit;
}
static void IntVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    VQ_UNUSED(col);
    q->ints[row] = cp != 0 ? cp->i : 0;
}
static void LongVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    VQ_UNUSED(col);
    q->longs[row] = cp != 0 ? cp->l : 0;
}
static void FloatVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    VQ_UNUSED(col);
    q->floats[row] = cp != 0 ? cp->f : 0;
}
static void DoubleVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    VQ_UNUSED(col);
    q->doubles[row] = cp != 0 ? cp->d : 0;
}
static void StringVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    const char **p = q->strings;
    VQ_UNUSED(col);
    free((void*) p[row]);
    p[row] = cp != 0 ? strcpy(malloc(strlen(cp->s)+1), cp->s) : 0;
}
static void BytesVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    vqSlot *p = q->slots;
    VQ_UNUSED(col);
    free(p[row].p);
    if (cp != 0) {
        p[row].p = memcpy(malloc(cp->x.y.i), cp->p, cp->x.y.i);
        p[row].x.y.i = cp->x.y.i;
    } else {
        p[row].p = 0;
        p[row].x.y.i = 0;
    }
}
static void ViewVecSetter (vqColumn q, int row, int col, const vqSlot *cp) {
    vqView orig = q->views[row];
    VQ_UNUSED(col);
    view_detach(q, row);
    q->views[row] = cp != 0 ? vq_incref(cp->v) : 0;
    view_attach(q, row);
    vq_decref(orig);
}

static void StringVecCleaner (vqColumn cp) {
    const char **p = cp->strings;
    int i;
    for (i = 0; i < vSize(cp); ++i)
        free((void*) p[i]);
}
static void BytesVecCleaner (vqColumn cp) {
    vqSlot *p = cp->slots;
    int i;
    for (i = 0; i < vSize(cp); ++i)
        free(p[i].p);
}
static void ViewVecCleaner (vqColumn cp) {
    vqView *p = cp->views;
    int i;
    for (i = 0; i < vSize(cp); ++i)
        vq_decref(p[i]);
}

static vqDispatch nvtab = {
    "nilvec", sizeof(vqData), -1, 0, NilVecGetter, NilVecSetter
};
static vqDispatch ivtab = {
    "intvec", sizeof(vqData), 4, 0, IntVecGetter, IntVecSetter
};
static vqDispatch pvtab = {
    "posvec", sizeof(vqData), 4, 0, PosVecGetter, IntVecSetter
};
static vqDispatch lvtab = {
    "longvec", sizeof(vqData), 8, 0, LongVecGetter, LongVecSetter 
};
static vqDispatch fvtab = {
    "floatvec", sizeof(vqData), 4, 0, FloatVecGetter, FloatVecSetter 
};
static vqDispatch dvtab = {
    "doublevec", sizeof(vqData), 8, 0, DoubleVecGetter, DoubleVecSetter 
};
static vqDispatch svtab = {
    "stringvec", sizeof(vqData), sizeof(void*), 
    StringVecCleaner, StringVecGetter, StringVecSetter
};
static vqDispatch bvtab = {
    "bytesvec", sizeof(vqData), sizeof(vqSlot), 
    BytesVecCleaner, BytesVecGetter, BytesVecSetter
};
static vqDispatch tvtab = {
    "viewvec", sizeof(vqData), sizeof(void*), 
    ViewVecCleaner, ViewVecGetter, ViewVecSetter
};

static void *new_datavec (vqType type, int rows) {
    int bytes;
    vqDispatch *vtab;
    vqColumn v;
    
    switch (type) {
        case VQ_nil:
            vtab = &nvtab; bytes = ((rows+31)/32) * sizeof(int); break;
        case VQ_int:
            vtab = &ivtab; bytes = rows * sizeof(int); break;
        case VQ_pos:
            vtab = &pvtab; bytes = rows * sizeof(int); break;
        case VQ_long:
            vtab = &lvtab; bytes = rows * sizeof(int64_t); break;
        case VQ_float:
            vtab = &fvtab; bytes = rows * sizeof(float); break;
        case VQ_double:
            vtab = &dvtab; bytes = rows * sizeof(double); break;
        case VQ_string:
            vtab = &svtab; bytes = rows * sizeof(const char*); break;
        case VQ_bytes:
            vtab = &bvtab; bytes = rows * sizeof(vqSlot); break;
        case VQ_view:
            vtab = &tvtab; bytes = rows * sizeof(vqView*); break;
        default: assert(0); return 0;
    }
    
    assert(vtab->headerbytes == sizeof(struct vqData_s));
    v = alloc_vec(vtab, bytes);
    vSize(v) = vLimit(v) = rows;
    return v;
}

static int *NewPosVec (int count) {
	return new_datavec(VQ_pos, count);
}

static vqType ViewColType (vqView v, int col) {
    return vq_getInt(vwMeta(v), col, 1, VQ_nil) & VQ_TYPEMASK;
}

static void ViewCleaner (vqColumn cp) {
    vqView v = cp->asview;
    int c, nc = vwCols(v);
    for (c = 0; c < nc; ++c) {
        vqColumn t = v->col[c].c;
        /* careful: col can be view in case of indirect views */
        if (t != 0 && t != cp) {
            if (vInfo(t).owner.col == cp)
                release_vec(t);
            else
                decColRef(t);
        }
    }
    vq_decref(vwMeta(v));
}
static vqDispatch vtab = {
    "view", sizeof(struct vqView_s), 0, ViewCleaner, 0, 0, DataVecReplacer
};
static vqView alloc_view (const vqDispatch *vtabp, vqView meta, int rows, int extra) {
    vqView v = alloc_vec(vtabp, vSize(meta) * sizeof(vqSlot) + extra);
    assert(vtabp->headerbytes == sizeof(struct vqView_s));
    vwEnv(v) = vwEnv(meta);
    vwAuxP(v) = (vqSlot*) v + vSize(meta);
    vSize(v) = rows;
    vwMeta(v) = vq_incref(meta);
    return v;
}
static void own_datavec (vqView parent, int colpos) {
    vqColumn v = parent->col[colpos].c;
    assert(v != 0);
    assert(vInfo(v).owner.col == 0);
    vInfo(v).owner.view = parent;
    vInfo(v).ownerpos = colpos;
    parent->col[colpos].c = v;
}
vqView vq_new (vqView meta, int rows) {
    vqView v = alloc_view(&vtab, meta, rows, 0);
    if (rows > 0) {
        int c, nc = vSize(meta);
        for (c = 0; c < nc; ++c) {
            v->col[c].p = new_datavec(ViewColType(v, c), rows);
            own_datavec(v, c);
        }
    }
    return v;
}

int vq_getInt (vqView t, int row, int col, int def) {
    vqSlot cell;
    cell.i = def;
    cell = vq_get(t, row, col, VQ_int, cell);
    return cell.i;
}
const char *vq_getString (vqView t, int row, int col, const char *def) {
    vqSlot cell;
    cell.s = def;
    cell = vq_get(t, row, col, VQ_string, cell);
    return cell.s;
}
vqView vq_getView (vqView t, int row, int col, vqView def) {
    vqSlot cell;
    cell.v = def;
    cell = vq_get(t, row, col, VQ_view, cell);
    return cell.v;
}

void vq_setEmpty (vqView t, int row, int col) {
    vqSlot cell;
    vq_set(t, row, col, VQ_nil, cell);
}
void vq_setInt (vqView t, int row, int col, int val) {
    vqSlot cell;
    cell.i = val;
    vq_set(t, row, col, VQ_int, cell);
}
void vq_setString (vqView t, int row, int col, const char *val) {
    vqSlot cell;
    cell.s = val;
    vq_set(t, row, col, val != 0 ? VQ_string : VQ_nil, cell);
}
void vq_setView (vqView t, int row, int col, vqView val) {
    vqSlot cell;
    cell.v = val;
    vq_set(t, row, col, val != 0 ? VQ_view : VQ_nil, cell);
}
void vq_setMetaRow (vqView m, int row, const char *nam, int typ, vqView sub) {
    vq_setString(m, row, 0, nam);
    vq_setInt(m, row, 1, typ);
    vq_setView(m, row, 2, sub != 0 ? sub : empty_meta(vwEnv(m)));
}

static void init_empty (vqEnv env) {
    vqView meta, mm;
    mm = alloc_vec(&vtab, 3 * sizeof(vqSlot));
    vwEnv(mm) = env;
    vSize(mm) = 3;
    vwMeta(mm) = mm;
    mm->col[0].v = new_datavec(VQ_string, 3);
    mm->col[1].v = new_datavec(VQ_int, 3);
    mm->col[2].v = new_datavec(VQ_view, 3);
    own_datavec(mm,0);
    own_datavec(mm,1);
    own_datavec(mm,2);
    
    meta = vq_new(mm, 0);
    push_view(meta);
    lua_setfield(env, LUA_REGISTRYINDEX, "vq.emv");

    vq_setMetaRow(mm, 0, "name", VQ_string, meta);
    vq_setMetaRow(mm, 1, "type", VQ_int, meta);
    vq_setMetaRow(mm, 2, "subv", VQ_view, meta);
}

int vq_isnil (vqView v, int row, int col) {
    vqSlot cell;
    if (col < 0 || col >= vwCols(v))
        return 1;
    cell = v->col[col];
    return getcell(row, &cell) == VQ_nil;
}
vqSlot vq_get (vqView v, int row, int col, vqType type, vqSlot def) {
    vqSlot cell;
    int nc = vwCols(v);
    VQ_UNUSED(type); /* TODO: check type arg */
    if (col < 0)
        col += nc;
    if (row < 0 || row >= vSize(v) || col < 0 || col >= nc)
        return def;
    cell = v->col[col];
    return getcell(row, &cell) == VQ_nil ? def : cell;
}
void vq_set (vqView v, int row, int col, vqType type, vqSlot val) {
    int nr = vSize(v), nc = vwCols(v);
    if (row < 0)
        row += nr;
    if (col < 0)
        col += nc;
#if 0
    /* TODO: decide about auto-extend behavior */
    if (row < 0 || row >= nr || col < 0 || col >= nc)
        return VQ_nil;
#endif
    if (vInfo(v).dispatch->setter == 0)
        v = v->col[col].v;
    assert(vInfo(v).dispatch->setter != 0);
    vInfo(v).dispatch->setter((vqColumn) v, row, col,
                                            type != VQ_nil ? &val : 0);
}
void vq_replace (vqView v, int start, int count, vqView data) {
    if (start < 0)
        start += vSize(v);
    assert(vInfo(v).dispatch->replacer != 0);
    vInfo(v).dispatch->replacer(v, start, count, data);
}

static vqSlot *checkrow (vqEnv env, int t) {
    return luaL_checkudata(env, t, "vq.row");
}

static vqView check_view (vqEnv env, int t) {
    switch (lua_type(env, t)) {
        case LUA_TNIL:      return 0;
        case LUA_TBOOLEAN:  return vq_new(empty_meta(env), lua_toboolean(env, t));
        case LUA_TNUMBER:   return vq_new(empty_meta(env), lua_tointeger(env, t));
        case LUA_TSTRING:   return desc2meta(env, lua_tostring(env, t), -1);
        case LUA_TTABLE:    return table2view(env, t);
    }
    return *(vqView*) luaL_checkudata(env, t, "vq.view");
}

static int check_pos (vqEnv env, int t) {
    int i = luaL_checkinteger(env, t);
    assert(i != 0);
    return i > 0 ? i - 1 : i;
}

static void push_pos (vqEnv env, int pos) {
    lua_pushinteger(env, pos + 1);
}

static int pushcell (vqEnv env, char c, vqSlot *cp) {
    if (cp == 0)
        c = 'N';
        
    switch (c) {
        case 'N':   lua_pushnil(env); break;
        case 'I':   lua_pushinteger(env, cp->i); break;
        case 'P':   push_pos(env, cp->i); break;
        case 'L':   lua_pushnumber(env, cp->l); break;
        case 'F':   lua_pushnumber(env, cp->f); break;
        case 'D':   lua_pushnumber(env, cp->d); break;
        case 'S':   lua_pushstring(env, cp->s); break;
        case 'B':   lua_pushlstring(env, cp->p, cp->x.y.i); break;
        case 'V':   push_view(cp->v); break;
    /* pseudo-types */
        case 'T':   lua_pushboolean(env, cp->i); break;
        default:    assert(0);
    }

    return 1;
}

static vqType check_cell (vqEnv env, int t, char c, vqSlot *cp) {
    if ('a' <= c && c <= 'z') {
        if (lua_isnoneornil(env, t)) {
            cp->v = cp->x.y.v = 0;
            return VQ_nil;
        }
        c += 'A'-'a';
    }
    switch (c) {
        case 'N':   cp->i = t; break;
        case 'I':
        case 'P':   if (lua_isnumber(env, t))
                        cp->i = luaL_checkinteger(env, t);
                    else
                        cp->i = vSize(check_view(env, t));
                    if (c == 'P')
                        --(cp->i);
                    break;
        case 'L':   cp->l = (int64_t) luaL_checknumber(env, t); break;
        case 'F':   cp->f = (float) luaL_checknumber(env, t); break;
        case 'D':   cp->d = luaL_checknumber(env, t); break;
        case 'S':   /* fall through */
        case 'B': {
            size_t n;
            cp->s = luaL_checklstring(env, t, &n);
            cp->x.y.i = n;
            break;
        }
        case 'V':   cp->v = check_view(env, t); break;
        default:    assert(0);
    }
    return char2type(c);
}

static vqView table2view (vqEnv env, int t) {
    vqSlot cell;
    vqView m, v;
    int r, rows, c, cols;
    lua_getfield(env, t, "meta");
    m = lua_isnil(env, -1) ? desc2meta(env, ":P", 2) : check_view(env, -1);
    lua_pop(env, 1);
    cols = vSize(m);
    rows = cols > 0 ? lua_objlen(env, t) / cols : 0;
    v = vq_new(m, rows);
    for (c = 0; c < cols; ++c) {
        vqType type = ViewColType(v, c);
        for (r = 0; r < rows; ++r) {
            vqType ty = type;
            lua_pushinteger(env, r * cols + c + 1);
            lua_gettable(env, t);
            ty = lua_isnil(env, -1) ? VQ_nil
                                  : check_cell(env, -1, VQ_TYPES[type], &cell);
            vq_set(v, r, c, ty, cell);
            lua_pop(env, 1);
        }
    }
    return v;
}

static void struct_helper (luaL_Buffer *B, vqView meta) {
    char buf[30];
    int c;
    for (c = 0; c < vSize(meta); ++c) {
        int type = vq_getInt(meta, c, 1, VQ_nil);
        if ((type & VQ_TYPEMASK) == VQ_view) {
            vqView m = vq_getView(meta, c, 2, 0);
            if (m != 0) {
                if (m == meta)
                    luaL_addchar(B, '^');
                else if (vSize(m) == 0)
                    luaL_addchar(B, 'V');
                else {
                    luaL_addchar(B, '(');
                    struct_helper(B, m);
                    luaL_addchar(B, ')');
                }
                continue;
            }
        }
        luaL_addstring(B, type2string(type, buf));
    }
}
static void push_struct (vqView v) {
    luaL_Buffer b;
    luaL_buffinit(vwEnv(v), &b);
    struct_helper(&b, vwMeta(v));
    luaL_pushresult(&b);
}

static int o_meta (vqEnv env) {
    return push_view(vwMeta(check_view(env, 1)));
}

static vqView IndirectView (const vqDispatch *vtabp, vqView meta, int rows, int extra) {
    int c, nc = vSize(meta);
    vqView v = alloc_view(vtabp, meta, rows, extra);
    vInfo(v).owner.view = v;
    vInfo(v).ownerpos = -1;
    for (c = 0; c < nc; ++c) {
        v->col[c].v = v;
        v->col[c].x.y.i = c;
    }
    return v;
}

static void RowColMapCleaner (vqColumn cp) {
    vqSlot *aux = vwAuxP(cp->asview);
    vq_decref(aux[0].v);
    vq_decref(aux[1].v);
    ViewCleaner(cp);
}
static vqType RowMapGetter (int row, vqSlot *cp) {
    int n;
    vqView v = cp->v;
    vqSlot *aux = vwAuxP(v);
    vqSlot map = aux[1].v->col[0];
    if (map.v != 0) {
        vqType type = getcell(row, &map);
        if (type == VQ_int || type == VQ_pos) {
            /* extra logic to support special maps with relative offsets < 0 */
            if (map.i < 0) {
                row += map.i;
                map = aux[1].v->col[0];
                getcell(row, &map);
            }
            row = map.i;
        }
    }
    v = aux[0].v;
    n = vSize(v);
    if (row >= n && n > 0)
        row %= n;
    *cp = v->col[cp->x.y.i];
    return getcell(row, cp);
}
static vqDispatch rowmaptab = {
    "rowmap", sizeof(struct vqView_s), 0, RowColMapCleaner, RowMapGetter
};
static vqView RowMapVop (vqView v, vqView map) {
    vqView t, m = vwMeta(map);
    vqType type = ViewColType(map, 0);
    vqSlot *aux;
    t = IndirectView(&rowmaptab, vwMeta(v), vSize(map), 2 * sizeof(vqSlot));
    aux = vwAuxP(t);
    aux[0].v = vq_incref(v);
    if (vSize(m) > 0 && (type == VQ_int || type == VQ_pos))
        aux[1].v = vq_incref(map);
    return t;
}

static int colbyname (vqView meta, const char* s) {
    int c, nc = vSize(meta);
    /* TODO: optimize this dumb linear search */
    for (c = 0; c < nc; ++c)
        if (strcmp(s, vq_getString(meta, c, 0, "")) == 0)
            return c;
    return luaL_error(vwEnv(meta), "column '%s' not found", s);
}
