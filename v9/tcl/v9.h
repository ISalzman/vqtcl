/* V9 public definitions
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#ifndef V9_H
#define V9_H

#include <stdint.h>

typedef struct ViewVector* V9View;

#define V9_TYPEMAP "NILFDSBV"
                
typedef enum V9Types {
    V9T_nil,
    V9T_int,
    V9T_long,
    V9T_float,
    V9T_double,
    V9T_string,
    V9T_bytes,
    V9T_view,
} V9Types;

typedef union V9Item {
    char         r[8];
    int          q[2];
    int          i;
    int64_t      l;
    float        f;
    double       d;
    void        *p;
    const char  *s;
    V9View       v;
    struct { void* p; int i; } c;
} V9Item;

/* v9_emit.c */
int V9_ViewEmit (V9View view, void* aux, void* (*fini)(void*,intptr_t), void* (*fdat)(void*,const void*,intptr_t));

/* v9_getter.c */
V9Item V9_Get (V9View v, int row, int col, V9Item* pdef);

/* v9_hashed.c */
V9View V9_IntersectMap (V9View keys, V9View view);
V9View V9_ExceptMap (V9View keys, V9View view);
V9View V9_RevIntersectMap (V9View keys, V9View view);
V9View V9_UniqMap (V9View view);

/* v9_indirect.c */
V9View V9_StepView (int count, int start, int step, int rate);
V9View V9_RowMapView (V9View data, V9View map);
V9View V9_ColMapView (V9View data, V9View map);
V9View V9_PlusView (V9View one, V9View two);
V9View V9_PairView (V9View one, V9View two);
V9View V9_TimesView (V9View data, int n);

/* v9_mapped.c */
V9View V9_MapDataAsView (const void* ptr, int len);

/* v9_setter.c */
V9View V9_Set (V9View v, int row, int col, V9Item* pitem);
V9View V9_Replace (V9View v, int offset, int count, V9View vnew);

/* v9_sort.c */
int V9_RowEqual (V9View v1, int r1, V9View v2, int r2);
int V9_RowCompare (V9View v1, int r1, V9View v2, int r2);
int V9_ViewCompare (V9View view1, V9View view2);
V9View V9_SortMap (V9View view);

/* v9_test.c */
const char* V9_AllocName (V9View v);
const char* V9_TypeName (V9View v);

/* v9_util.c */
V9View V9_AddRef (V9View v);
V9View V9_Release (V9View v);
V9View V9_Meta (V9View v);
int V9_Size (V9View v);
int V9_GetInt (V9View v, int row, int col);
const char* V9_GetString (V9View v, int row, int col);
V9View V9_GetView (V9View v, int row, int col);
V9Types V9_CharAsType (char type);

/* v9_view.c */
V9View V9_NewDataView (V9View meta, int rows);
int V9_MetaAsDescLength (V9View meta);
char* V9_MetaAsDesc (V9View meta, char* buf);
V9View V9_DescAsMeta (const char* s, const char* end);

#endif
