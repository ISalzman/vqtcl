/* V9 core view support
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include <string.h>

static void DataViewCleaner (Vector vp) {
	V9View v = (V9View) vp;
	int c, ncols = Head(v->meta).count;
	for (c = 0; c < ncols; ++c)
		DecRef(v->col[c].c.p);
	V9_Release(v->meta);
}

static void DataViewSetter (Vector vp, int row, int col, const V9Item* pitem) {
	V9View v = (V9View) vp;
    Unshare((Vector*) &v->col[col].c.p);
    Vector cvp = v->col[col].c.p;
    assert(Head(cvp).type->setter != 0);
    Head(cvp).type->setter(cvp, row, col, pitem);
}

static Vector DataViewDuplicator (Vector vp) {
	V9View v = (V9View) vp;
	int c, ncols = Head(v->meta).count;
    int bytes = sizeof *v + ncols * sizeof(V9Item);

    V9_AddRef(v->meta);
	for (c = 0; c < ncols; ++c)
	    if (v->col[c].c.p != 0)
		    IncRef(v->col[c].c.p);

    Vector vnew = NewInlineVector(bytes);
    memcpy(vnew - 1, vp - 1, bytes + sizeof *vp);
    Head(vnew).shares = -1;
    return vnew;
}

static struct VectorType vtView = {
    "view", -1, 
    DataViewCleaner, 0, DataViewSetter, DataViewReplacer, DataViewDuplicator,
};

V9View V9_NewDataView (V9View meta, int nrows) {
	int c, ncols = meta != 0 ? Head(meta).count : 0;
	V9View v = (V9View) NewInlineVector(sizeof *v + ncols * sizeof(V9Item));
	Head(v).count = nrows;
	Head(v).type = &vtView;
	v->meta = V9_AddRef(meta);

	for (c = 0; c < ncols; ++c) {
        V9Types type = V9_GetInt(meta, c, 1);
		v->col[c].c.p = nrows > 0 ? IncRef(NewDataVec(type, nrows)) : 0;
		v->col[c].c.i = -1;
	}

    return v;
}

V9View IndirectView (V9View meta, int nrows, VectorType* type, int bytes) {
	int c, ncols = meta != 0 ? Head(meta).count : 0;
	V9View v = (V9View) NewInlineVector(sizeof *v + ncols * sizeof(V9Item) + bytes);
	Head(v).count = nrows;
	Head(v).type = type;
    v->extra = v->col + ncols;
	v->meta = V9_AddRef(meta);

	for (c = 0; c < ncols; ++c) {
		v->col[c].c.p = v;
		v->col[c].c.i = c;
	}

    return v;
}

static void SetMetaRow (V9View meta, int row, const char* name, int type, V9View subv) {
    V9Item item;
    item.s = name;
    V9_Set(meta, row, 0, &item);
    item.i = type;
    V9_Set(meta, row, 1, &item);
    item.v = subv;
    V9_Set(meta, row, 2, &item);
}

V9View EmptyMeta () {
    static V9View meta = 0;
    if (meta == 0) {
        V9View mm = (V9View) NewInlineVector(sizeof *mm + 3 * sizeof(V9Item));
		Head(mm).count = 3;
		Head(mm).type = &vtView;
        mm->meta = mm;
        mm->col[0].c.p = IncRef(NewDataVec(V9T_string, 3));
        mm->col[1].c.p = IncRef(NewDataVec(V9T_int, 3));
        mm->col[2].c.p = IncRef(NewDataVec(V9T_view, 3));
        
        // won't access mm's types column, which hasn't been filled in yet
        meta = V9_NewDataView(mm, 0);

        SetMetaRow(mm, 0, "name", V9T_string, meta);
        SetMetaRow(mm, 1, "type", V9T_int, meta);
        SetMetaRow(mm, 2, "subv", V9T_view, meta);
    }
    return meta;
}

// used as pre-flight, to determine the required buffer size for V9_MetaAsDesc
int V9_MetaAsDescLength (V9View meta) {
    int r, nrows = Head(meta).count, len = 0;
    for (r = 0; r < nrows; ++r) {
        if (r > 0)
            ++len;
        len += strlen(V9_GetString(meta, r, 0)) + 2;
        if (V9_GetInt(meta, r, 1) == V9T_view) {
	        V9View subv = V9_GetView(meta, r, 2);
			if (Head(subv).count > 0)
				len += V9_MetaAsDescLength(subv);
		}
    }
	return len;
}

char* V9_MetaAsDesc (V9View meta, char* buf) {
	V9Types type;
    int r, nrows = Head(meta).count;
    const char *name;
    V9View subv;

    for (r = 0; r < nrows; ++r) {
        if (r > 0)
            *buf++ = ',';

        name = V9_GetString(meta, r, 0);
        type = V9_GetInt(meta, r, 1);
        subv = V9_GetView(meta, r, 2);

		while (*name)
			*buf++ = *name++;
        if (type == V9T_view && Head(subv).count > 0) {
            *buf++ = '[';
            buf = V9_MetaAsDesc(subv, buf);
            *buf++ = ']';
        } else {
            *buf++ = ':';
            *buf++ = V9_TYPEMAP[type];
        }
    }

	*buf = 0;
	return buf;
}

static const char* ScanDesc (const char* s, const char* end, V9View* pmeta) {
    *pmeta = EmptyMeta();
    if (s >= end)
        return s;
    
    // pre-flight scan to count the columns, i.e. rows in the meta-view
    const char* t = s;
    int level = 0, nrows = 1;
    while (t < end && level >= 0)
        switch (*t++) {
            case '[':   ++level; break;
            case ']':   --level; break;
            case ',':   if (level == 0) ++nrows; break;
        }

    // parse each column description and fill in the meta-view rows
    V9View v = V9_NewDataView((*pmeta)->meta, nrows);
    int r;
    for (r = 0; r < nrows; ++r) {
        const char* p = s;
		while (s < end && strchr(":,[]", *s) == 0)
			++s;
        int n = s - p;
        
        V9Types type = V9T_string;
        if (s < end && *s == ':') {
            ++s;
            type = V9_CharAsType(*s++);
        }
        
        V9View subv = 0;
        if (s < end && *s == '[') {
            s = ScanDesc(++s, end, &subv);
            if (s == 0 || (s < end && *s++ != ']')) {
                V9_Release(subv);
                goto FAIL;
            }
            type = V9T_view;
        }

        // use temp buffer to set up zero-terminated name
        Buffer buf;
        InitBuffer(&buf);
        AddToBuffer(&buf, p, n);
        ADD_CHAR_TO_BUF(buf, 0);
        SetMetaRow(v, r, BufferAsPtr(&buf, 1), type, subv);
        ReleaseBuffer(&buf, 0);        

        if (s >= end)
            break;
        if (*s != ',' && *s != ']')
            goto FAIL;
        ++s;
    }    
    *pmeta = v;
    return s;
    
FAIL:
    V9_Release(v);
    *pmeta = 0;
    return 0;
}

V9View V9_DescAsMeta (const char* s, const char* end) {
    if (end == 0)
        end = s + strlen(s);
    V9View v;
const char* t = ScanDesc(s, end, &v);
assert(t == end);
    return ScanDesc(s, end, &v) == end ? v : 0;
}
