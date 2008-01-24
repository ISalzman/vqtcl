/*
 * view.c - Implementation of views.
 */

#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "wrap_gen.h"

int ViewSize (View_p view) {
    Seq_p seq = ViewCol(view, 0).seq;
    return seq != NULL ? seq->count : 0;
}

char GetColType (View_p meta, int col) {
    /* TODO: could be a #define */
    return *GetViewItem(meta, col, MC_type, IT_string).s;
}

static void ViewCleaner (View_p view) {
    int c;
    for (c = 0; c <= ViewWidth(view); ++c)
        DecRefCount(ViewCol(view, c).seq);
}

static ItemTypes ViewGetter (int row, Item_p item) {
    item->c = ViewCol(item->c.seq, row);
    return IT_column;
}

static struct SeqType ST_View = { "view", ViewGetter, 010, ViewCleaner };

static View_p ForceMetaView(View_p meta) {
    View_p newmeta;
    Column orignames, names;
    
    /* this code is needed so we can always do a hash lookup on name column */
    
    orignames = ViewCol(meta, MC_name);
    names = ForceStringColumn(orignames);
    
    if (names.seq == orignames.seq)
        return meta;
        
    /* need to construct a new meta view with the adjusted names column */
    newmeta = NewView(V_Meta(meta));

    SetViewCols(newmeta, MC_name, 1, names);
    SetViewCols(newmeta, MC_type, 1, ViewCol(meta, MC_type));
    SetViewCols(newmeta, MC_subv, 1, ViewCol(meta, MC_subv));

    return newmeta;
}

View_p NewView (View_p meta) {
    int c, cols, bytes;
    View_p view;

    /* a view must always be able to store at least one column */
    cols = ViewSize(meta);
    bytes = (cols + 1) * (sizeof(Column) + 1);

    view = NewSequence(cols, &ST_View, bytes);
    /* data[0] points to types string (allocated inline after the columns) */
    /* data[1] is the meta view */
    V_Types(view) = (char*) (V_Cols(view) + cols + 1);
    V_Meta(view) = IncRefCount(ForceMetaView(meta));

    /* TODO: could optimize by storing the types with the meta view instead */
    for (c = 0; c < cols; ++c)
        V_Types(view)[c] = CharAsItemType(GetColType(meta, c));

    return view;
}

void SetViewCols (View_p view, int first, int count, Column src) {
    int c;

    AdjustSeqRefs(src.seq, count);
    for (c = first; c < first + count; ++c) {
        DecRefCount(ViewCol(view, c).seq);
        ViewCol(view, c) = src;
    }
}

void SetViewSeqs (View_p view, int first, int count, Seq_p src) {
    SetViewCols(view, first, count, SeqAsCol(src));
}

View_p IndirectView (View_p meta, Seq_p seq) {
    int c, cols;
    View_p result;

    cols = ViewSize(meta);
    if (cols == 0)
        ++cols;
        
    result = NewView(meta);
    SetViewSeqs(result, 0, cols, seq);

    for (c = 0; c < cols; ++c)
        ViewCol(result, c).pos = c;

    return result;
}

View_p EmptyMetaView (void) {
    Shared_p sh = GetShared();
    
    if (sh->empty == NULL) {
        int bytes;
        View_p meta, subs [MC_limit];
        Seq_p tempseq;

        /* meta is recursively defined, so we can't use NewView here */
        bytes = (MC_limit + 1) * (sizeof(Column) + 1);
        meta = NewSequence(MC_limit, &ST_View, bytes);
        V_Meta(meta) = IncRefCount(meta); /* circular */
    
        V_Types(meta) = (char*) (V_Cols(meta) + MC_limit + 1);
        V_Types(meta)[0] = V_Types(meta)[1] = IT_string;
        V_Types(meta)[2] = IT_view;

        /* initialize all but last columns of meta with static ptr contents */
        tempseq = NewStrVec(1);
        AppendToStrVec("name", -1, tempseq);
        AppendToStrVec("type", -1, tempseq);
        AppendToStrVec("subv", -1, tempseq);
        SetViewSeqs(meta, MC_name, 1, FinishStrVec(tempseq));

        tempseq = NewStrVec(1);
        AppendToStrVec("S", -1, tempseq);
        AppendToStrVec("S", -1, tempseq);
        AppendToStrVec("V", -1, tempseq);
        SetViewSeqs(meta, MC_type, 1, FinishStrVec(tempseq));

        /* same structure as meta but no rows */
        sh->empty = NewView(meta);

        /* initialize last column of meta, now that empty exists */
        subs[MC_name] = subs[MC_type] = subs[MC_subv] = sh->empty;
        SetViewSeqs(meta, MC_subv, 1, NewSeqVec(IT_view, subs, MC_limit));
    }
    
    return sh->empty;
}

View_p NoColumnView (int rows) {
    View_p result = NewView(EmptyMetaView());
    SetViewSeqs(result, 0, 1, NewSequence(rows, PickIntGetter(0), 0));
    return result;
}

View_p ColMapView (View_p view, Column mapcol) {
    int c, cols;
    const int *map;
    View_p result;

    map = (const int*) mapcol.seq->data[0].p;
    cols = mapcol.seq->count;

    if (cols == 0)
        return NoColumnView(ViewSize(view));

    result = NewView(RemapSubview(V_Meta(view), mapcol, 0, cols));

    for (c = 0; c < cols; ++c)
        SetViewCols(result, c, 1, ViewCol(view, map[c]));

    return result;
}

View_p ColOmitView (View_p view, Column omitcol) {
    int c, d, *map;
    Seq_p mapcol;
    
    mapcol = NewIntVec(ViewWidth(view), &map);

    /* TODO: make this robust, may have to add bounds check */
    for (c = 0; c < omitcol.seq->count; ++c)
        map[GetColItem(c, omitcol, IT_int).i] = 1;
    
    for (c = d = 0; c < mapcol->count; ++c)
        if (!map[c])
            map[d++] = c;
            
    mapcol->count = d; /* TODO: truncate unused area */
    return ColMapView(view, SeqAsCol(mapcol));
}

View_p OneColView (View_p view, int col) {
    View_p result;

    result = NewView(StepView(V_Meta(view), 1, col, 1, 1));
    SetViewCols(result, 0, 1, ViewCol(view, col));

    return result;
}

View_p FirstView (View_p view, int count) {
    if (count >= ViewSize(view))
        return view;
    return StepView(view, count, 0, 1, 1);
}

View_p LastView (View_p view, int count) {
    int rows = ViewSize(view);
    if (count >= rows)
        return view;
    return StepView(view, count, rows - count, 1, 1);
}

View_p TakeView (View_p view, int count) {
    if (count >= 0)
        return StepView(view, count, 0, 1, 1);
    else
        return StepView(view, -count, -1, 1, -1);
}

View_p CloneView (View_p view) {
    return NewView(V_Meta(view));
}

View_p PairView (View_p view1, View_p view2) {
    int c, rows1 = ViewSize(view1), rows2 = ViewSize(view2);
    View_p meta, result;

    if (rows2 < rows1)
        view1 = FirstView(view1, rows2);
    else if (rows1 < rows2)
        view2 = FirstView(view2, rows1);
    else if (ViewWidth(view2) == 0)
        return view1;
    else if (ViewWidth(view1) == 0)
        return view2;

    meta = ConcatView(V_Meta(view1), V_Meta(view2));
    result = NewView(meta);
    
    for (c = 0; c < ViewWidth(view1); ++c)
        SetViewCols(result, c, 1, ViewCol(view1, c));
    for (c = 0; c < ViewWidth(view2); ++c)
        SetViewCols(result, ViewWidth(view1) + c, 1, ViewCol(view2, c));
        
    return result;
}

static void ViewStructure (View_p meta, Buffer_p buffer) {
    int c, cols = ViewSize(meta);
    
    for (c = 0; c < cols; ++c) {
        char type = GetColType(meta, c);
        
        if (type == 'V') {
            View_p submeta = GetViewItem(meta, c, MC_subv, IT_view).v;
            
            if (ViewSize(submeta) > 0) {
                AddToBuffer(buffer, "(", 1);
                ViewStructure(submeta, buffer);
                AddToBuffer(buffer, ")", 1);
                continue;
            }
        }
        AddToBuffer(buffer, &type, 1);
    }
}

int MetaIsCompatible (View_p meta1, View_p meta2) {
    if (meta1 != meta2) {
        int c, cols = ViewSize(meta1);

        if (cols != ViewSize(meta2))
            return 0;
        
        for (c = 0; c < cols; ++c) {
            char type = GetColType(meta1, c);
        
            if (type != GetColType(meta2, c))
                return 0;
            
            if (type == 'V') {
                View_p v1 = GetViewItem(meta1, c, MC_subv, IT_view).v;
                View_p v2 = GetViewItem(meta2, c, MC_subv, IT_view).v;

                if (!MetaIsCompatible(v1, v2))
                    return 0;
            }
        }
    }
    
    return 1;
}

int ViewCompare (View_p view1, View_p view2) {
    int f, r, rows1, rows2;
    
    if (view1 == view2)
        return 0;
    
    f = ViewCompare(V_Meta(view1), V_Meta(view2));
    if (f != 0)
        return f;
        
    rows1 = ViewSize(view1);
    rows2 = ViewSize(view2);
    
    for (r = 0; r < rows1; ++r) {
        if (r >= rows2)
            return 1;
    
        f = RowCompare(view1, r, view2, r);
        if (f != 0)
            return f < 0 ? -1 : 1;
    }
        
    return rows1 < rows2 ? -1 : 0;
}

View_p MakeIntColMeta (const char *name) {
    View_p result, subv;
    Seq_p names, types, subvs;
    
    names = NewStrVec(1);
    AppendToStrVec(name, -1, names);

    types = NewStrVec(1);
    AppendToStrVec("I", -1, types);

    subv = EmptyMetaView();
    subvs = NewSeqVec(IT_view, &subv, 1);
    
    result = NewView(V_Meta(EmptyMetaView()));
    SetViewSeqs(result, MC_name, 1, FinishStrVec(names));
    SetViewSeqs(result, MC_type, 1, FinishStrVec(types));
    SetViewSeqs(result, MC_subv, 1, subvs);
    return result;
}

View_p TagView (View_p view, const char* name) {
    View_p tagview;
    
    tagview = NewView(MakeIntColMeta(name));
    SetViewCols(tagview, 0, 1, NewIotaColumn(ViewSize(view)));
    
    return PairView(view, tagview);
}

View_p DescAsMeta (const char** desc, const char* end) {
    int c, len, cols = 0, namebytes = 0;
    char sep, *buf, *ptr, **strings;
    Seq_p *views, tempseq;
    struct Buffer names, types, subvs;
    View_p result, empty;

    InitBuffer(&names);
    InitBuffer(&types);
    InitBuffer(&subvs);
    
    len = end - *desc;
    buf = memcpy(malloc(len+1), *desc, len);
    buf[len] = ',';
    
    empty = EmptyMetaView();
    result = NewView(V_Meta(empty));
    
    for (ptr = buf; ptr < buf + len; ++ptr) {
        const char *s = ptr;
        while (strchr(":,[]", *ptr) == 0)
            ++ptr;
        sep = *ptr;
        *ptr = 0;

        ADD_PTR_TO_BUF(names, s);
        namebytes += ptr - s + 1;
         
        switch (sep) {

            case '[':
                ADD_PTR_TO_BUF(types, "V");
                ++ptr;
                ADD_PTR_TO_BUF(subvs, DescAsMeta((const char**) &ptr, buf + len));
                sep = *++ptr;
                break;

            case ':':
                ptr += 2;
                sep = *ptr;
                *ptr = 0;
                ADD_PTR_TO_BUF(types, ptr - 1);
                ADD_PTR_TO_BUF(subvs, empty);
                break;

            default:
                ADD_PTR_TO_BUF(types, "S");
                ADD_PTR_TO_BUF(subvs, empty);
                break;
        }
        
        ++cols;
        if (sep != ',')
            break;
    }

    strings = BufferAsPtr(&names, 1);
    tempseq = NewStrVec(1);
    for (c = 0; c < cols; ++c)
        AppendToStrVec(strings[c], -1, tempseq);
    SetViewSeqs(result, MC_name, 1, FinishStrVec(tempseq));
    ReleaseBuffer(&names, 0);
    
    strings = BufferAsPtr(&types, 1);
    tempseq = NewStrVec(1);
    for (c = 0; c < cols; ++c)
        AppendToStrVec(strings[c], -1, tempseq);
    SetViewSeqs(result, MC_type, 1, FinishStrVec(tempseq));
    ReleaseBuffer(&types, 0);
    
    views = BufferAsPtr(&subvs, 1);
    tempseq = NewSeqVec(IT_view, views, cols);
    SetViewSeqs(result, MC_subv, 1, tempseq);
    ReleaseBuffer(&subvs, 0);
    
    free(buf);
    *desc += ptr - buf;
    return result;
}

View_p DescToMeta (const char *desc) {
    int len;
    const char *end;
    View_p meta;

    len = strlen(desc);
    end = desc + len;
    
    meta = DescAsMeta(&desc, end);
    if (desc < end)
        return NULL;

    return meta;
}

ItemTypes DataCmd_VX (Item args[]) {
    View_p meta, view;
    int c, cols, objc;
    const Object_p *objv;
    Column column;

    meta = args[0].v;
    cols = ViewSize(meta);
    
    objv = (const void*) args[1].u.ptr;
    objc = args[1].u.len;

    if (objc != cols) {
        args->e = EC_wnoa;
        return IT_error;
    }

    view = NewView(meta);
    for (c = 0; c < cols; ++c) {
        column = CoerceColumn(ViewColType(view, c), objv[c]);
        if (column.seq == NULL)
            return IT_unknown;
            
        SetViewCols(view, c, 1, column);
    }

    args->v = view;
    return IT_view;
}

ItemTypes StructDescCmd_V (Item args[]) {
    struct Buffer buffer;
    Item item;
    
    InitBuffer(&buffer);
    MetaAsDesc(V_Meta(args[0].v), &buffer);
    AddToBuffer(&buffer, "", 1);
    item.s = BufferAsPtr(&buffer, 1);
    args->o = ItemAsObj(IT_string, &item);
    ReleaseBuffer(&buffer, 0);
    return IT_object;
}

ItemTypes StructureCmd_V (Item args[]) {
    struct Buffer buffer;
    Item item;
    
    InitBuffer(&buffer);
    ViewStructure(V_Meta(args[0].v), &buffer);
    AddToBuffer(&buffer, "", 1);
    item.s = BufferAsPtr(&buffer, 1);
    args->o = ItemAsObj(IT_string, &item);
    ReleaseBuffer(&buffer, 0);
    return IT_object;
}
