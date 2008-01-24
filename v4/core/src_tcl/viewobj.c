/*
 * viewobj.c - Implementation of view objects in Tcl.
 */

#include "ext_tcl.h"

#define DEP_DEBUG 0

#define OBJ_TO_VIEW_P(o) ((o)->internalRep.twoPtrValue.ptr1)
#define OBJ_TO_ORIG_P(o) ((o)->internalRep.twoPtrValue.ptr2)

#define OBJ_TO_ORIG_CNT(o)  (((Seq_p) OBJ_TO_ORIG_P(o))->OR_objc)
#define OBJ_TO_ORIG_VEC(o)  ((Object_p*) ((Seq_p) OBJ_TO_ORIG_P(o) + 1))

#define OR_depx     data[0].o
#define OR_objc     data[1].i
#define OR_depc     data[2].i
#define OR_depv     data[3].p

/* forward */
static void SetupViewObj (Object_p, View_p, int objc, Object_p *objv, Object_p);
static void ClearViewObj (Object_p obj);

static void FreeViewIntRep (Tcl_Obj *obj) {
    PUSH_KEEP_REFS

    Assert(((Seq_p) OBJ_TO_ORIG_P(obj))->refs == 1);
    ClearViewObj(obj);
    
    DecRefCount(OBJ_TO_VIEW_P(obj));
    DecRefCount(OBJ_TO_ORIG_P(obj));
    
    POP_KEEP_REFS
}

static void DupViewIntRep (Tcl_Obj *src, Tcl_Obj *dup) {
    Assert(0);
}

static void UpdateViewStrRep (Tcl_Obj *obj) {
    int length;
    const char *string;
    Object_p origobj;
    PUSH_KEEP_REFS
    
    /* TODO: avoid creating a Tcl list and copying the string, use a buffer */
    origobj = Tcl_NewListObj(OBJ_TO_ORIG_CNT(obj), OBJ_TO_ORIG_VEC(obj));
    string = Tcl_GetStringFromObj(origobj, &length);

    obj->bytes = strcpy(ckalloc(length+1), string);
    obj->length = length;

    DecObjRef(origobj);

    POP_KEEP_REFS
}

static char *RefTracer (ClientData cd, Tcl_Interp *interp, const char *name1, const char *name2, int flags) {
    Object_p obj = (Object_p) cd;
    
#if DEBUG+0
    DbgIf(DBG_trace) printf("       rt %p var: %s\n", (void*) obj, name1);
#endif

    if (flags && TCL_TRACE_WRITES) {
        Tcl_UntraceVar2(Interp(), name1, name2,
                        TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
                        RefTracer, obj);
    }

    InvalidateView(obj);
    DecObjRef(obj);
    return NULL;
}

static int SetViewFromAnyRep (Tcl_Interp *interp, Tcl_Obj *obj) {
    int e = TCL_ERROR, objc, rows;
    Object_p *objv;
    View_p view;
    
    if (Tcl_ListObjGetElements(interp, obj, &objc, &objv) != TCL_OK)
        return TCL_ERROR;

    PUSH_KEEP_REFS

    switch (objc) {

        case 0:
            view = EmptyMetaView();
            SetupViewObj(obj, view, objc, objv, NULL);
            break;

        case 1:
            if (Tcl_GetIntFromObj(interp, objv[0], &rows) == TCL_OK &&
                                                                rows >= 0) {
                view = NoColumnView(rows);
                SetupViewObj(obj, view, objc, objv, NULL);
            } else {
                Object_p o;
                const char *var = Tcl_GetString(objv[0]) + 1;
                
                if (var[-1] != '@')
                    goto FAIL;
                    
#if DEBUG+0
                DbgIf(DBG_trace) printf("svfar %p var: %s\n", (void*) obj, var);
#endif
                o = (Object_p) Tcl_VarTraceInfo(Interp(), var, TCL_GLOBAL_ONLY,
                                                            RefTracer, NULL);
                if (o == NULL) {
                    o = Tcl_GetVar2Ex(interp, var, NULL,
                                        TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
                    if (o == NULL)
                        goto FAIL;

                    o = NeedMutable(o);
                    Assert(o != NULL);
                    
                    if (Tcl_TraceVar2(interp, var, 0,
                            TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
                                            RefTracer, IncObjRef(o)) != TCL_OK)
                        goto FAIL;
                }
                
                view = ObjAsView(o);
                Assert(view != NULL);

                SetupViewObj(obj, view, objc, objv, /*o*/ NULL);
            }
            
            break;

        default: {
            Object_p cmd;
            Tcl_SavedResult state;

            Assert(interp != NULL);
            Tcl_SaveResult(interp, &state);

            cmd = IncObjRef(Tcl_DuplicateObj(obj));
            view = NULL;

            /* def -> cmd namespace name conv can prob be avoided in Tcl 8.5 */
            if (AdjustCmdDef(cmd) == TCL_OK) {
                int ac;
                Object_p *av;
                Tcl_ListObjGetElements(NULL, cmd, &ac, &av);
                /* don't use Tcl_EvalObjEx, it forces a string conversion */
                if (Tcl_EvalObjv(interp, ac, av, TCL_EVAL_GLOBAL) == TCL_OK) {
                    /* result to view, may call EvalIndirectView recursively */
                    view = ObjAsView(Tcl_GetObjResult(interp));
                }
            }

            DecObjRef(cmd);

            if (view == NULL) {
                Tcl_DiscardResult(&state);
                goto FAIL;
            }

            SetupViewObj(obj, view, objc, objv, NULL);
            Tcl_RestoreResult(interp, &state);
        }
    }

    e = TCL_OK;

FAIL:
    POP_KEEP_REFS
    return e;
}

Tcl_ObjType f_viewObjType = {
    "view", FreeViewIntRep, DupViewIntRep, UpdateViewStrRep, SetViewFromAnyRep
};

View_p ObjAsView (Object_p obj) {
    if (Tcl_ConvertToType(Interp(), obj, &f_viewObjType) != TCL_OK)
        return NULL;
    return OBJ_TO_VIEW_P(obj);
}

static void AddDependency (Object_p obj, Object_p child) { 
    Object_p *deps;
    Seq_p origin = OBJ_TO_ORIG_P(obj);
    
#if DEBUG+0
    DbgIf(DBG_deps) printf(" +dep %p %p\n", (void*) obj, (void*) child);
#endif

    if (origin->OR_depv == NULL) 
        origin->OR_depv = calloc(10, sizeof(Object_p));
    
    Assert(origin->OR_depc < 10);
    deps = origin->OR_depv;
    deps[origin->OR_depc++] = child;
}

static void RemoveDependency (Object_p obj, Object_p child) {    
    Object_p *deps;
    Seq_p origin = OBJ_TO_ORIG_P(obj);
    
#if DEBUG+0
    DbgIf(DBG_deps) printf(" -dep %p %p\n", (void*) obj, (void*) child);
#endif

    deps = origin->OR_depv;
    Assert(deps != NULL);
    
    if (deps != NULL) {
        int i;

        /* removed from end in case we're called from the invalidation loop */
        for (i = origin->OR_depc; --i >= 0; )
            if (deps[i] == child)
                break;
        /*printf("  -> i %d n %d\n", i, origin->OR_depc);*/
        
        Assert(i < origin->OR_depc);
        deps[i] = deps[--origin->OR_depc];
            
        if (origin->OR_depc == 0) {
            free(deps);
            origin->OR_depv = NULL;
        }
    }
}

void InvalidateView (Object_p obj) {
    Object_p *deps;
    Seq_p origin = OBJ_TO_ORIG_P(obj);
    
#if DEP_DEBUG+0
    printf("inval %p deps %d refs %d\n",
                    (void*) obj, origin->OR_depc, obj->refCount);
#endif

    if (obj->typePtr != &f_viewObjType)
        return;
    
    deps = origin->OR_depv;
    if (deps != NULL) {
        /* careful, RemoveDependency will remove the item during iteration */
        while (origin->OR_depc > 0) {
            int last = origin->OR_depc - 1;
            InvalidateView(deps[last]);
            Assert(origin->OR_depc <= last); /* make sure count went down */
        }
        Assert(origin->OR_depv == NULL);
    }

#if 1
    Assert(obj->typePtr == &f_viewObjType);
    Tcl_GetString(obj);
    FreeViewIntRep(obj);
    obj->typePtr = NULL;
#else
    /* FIXME: hack to allow changes to list, even when the object is shared! */
    { Object_p nobj = Tcl_NewListObj(OBJ_TO_ORIG_CNT(obj),
                                        OBJ_TO_ORIG_VEC(obj));
        DecRefCount(OBJ_TO_VIEW_P(obj));
        DecRefCount(origin);
        obj->typePtr = nobj->typePtr;
        obj->internalRep = nobj->internalRep;
        nobj->typePtr = NULL;
        DecObjRef(nobj);
        /*Tcl_InvalidateStringRep(obj);*/
    }
#endif
}

#if 0
static void OriginCleaner (Seq_p seq) {
    int i, items = seq->OR_objc;
    const Object_p *objv = (const void*) (seq + 1);

    for (i = 0; i < items; ++i)
        DecObjRef(objv[i]);

    Assert(seq->OR_depc == 0);
    Assert(seq->OR_depv == NULL);
}

static ItemTypes OriginGetter (int row, Item_p item) {
    const Object_p *objv = (const void*) (item->c.seq + 1);
    item->o = objv[row];
    return IT_object;
}
#endif

static struct SeqType ST_Origin = { "origin", NULL, 0, NULL };

static void SetupViewObj (Object_p obj, View_p view, int objc, Object_p *objv, Object_p extradep) {
    int i;
    Object_p *vals;
    Seq_p origin;
    
    origin = NewSequenceNoRef(objc, &ST_Origin, objc * sizeof(Object_p));
    /* data[0] is a pointer to the reference for "@..." references */
    /* data[1] has the number of extra bytes, i.e. objc * sizeof (Seq_p) */
    /* data[2] is the dependency count */
    /* data[3] is the dependency vector if data[2].i > 0 */
    origin->OR_depx = extradep;
    origin->OR_objc = objc;
    vals = (void*) (origin + 1);
    
    for (i = 0; i < objc; ++i)
        vals[i] = IncObjRef(objv[i]);

    if (obj->typePtr != NULL && obj->typePtr->freeIntRepProc != NULL)
        obj->typePtr->freeIntRepProc(obj);
    
    OBJ_TO_VIEW_P(obj) = IncRefCount(view);
    OBJ_TO_ORIG_P(obj) = IncRefCount(origin);
    obj->typePtr = &f_viewObjType;
    
    for (i = 0; i < objc; ++i)
        if (vals[i]->typePtr == &f_viewObjType)
            AddDependency(vals[i], obj);
            
    if (extradep != NULL)
        AddDependency(extradep, obj);
}

static void ClearViewObj (Object_p obj) {
    int i, objc;
    Object_p *objv;
    Seq_p origin = OBJ_TO_ORIG_P(obj);
    
    /* can't have any child dependencies at this stage */
    Assert(origin->OR_depc == 0);
    Assert(origin->OR_depv == NULL);

    objc = OBJ_TO_ORIG_CNT(obj);
    objv = OBJ_TO_ORIG_VEC(obj);
    
    for (i = 0; i < objc; ++i) {
        if (objv[i]->typePtr == &f_viewObjType)
            RemoveDependency(objv[i], obj);
        DecObjRef(objv[i]);
    }
        
    if (origin->OR_depx != NULL)
        RemoveDependency(origin->OR_depx, obj);
}

static Object_p MetaViewAsObj (View_p meta) {
    char typeCh;
    int width, rowNum;
    Object_p result, fieldobj;
    View_p subv;
    Column names, types, subvs;

    names = ViewCol(meta, MC_name);
    types = ViewCol(meta, MC_type);
    subvs = ViewCol(meta, MC_subv);

    result = Tcl_NewListObj(0, NULL);

    width = ViewSize(meta);
    for (rowNum = 0; rowNum < width; ++rowNum) {
        fieldobj = Tcl_NewStringObj(GetColItem(rowNum, names, IT_string).s, -1);

        /* this ignores all but the first character of the type */
        typeCh = *GetColItem(rowNum, types, IT_string).s;

        switch (typeCh) {

            case 'S':
                break;

            case 'V':
                subv = GetColItem(rowNum, subvs, IT_view).v;
                fieldobj = Tcl_NewListObj(1, &fieldobj);
                Tcl_ListObjAppendElement(NULL, fieldobj, MetaViewAsObj(subv));
                break;

            default:
                Tcl_AppendToObj(fieldobj, ":", 1);
                Tcl_AppendToObj(fieldobj, &typeCh, 1);
                break;
        }

        Tcl_ListObjAppendElement(NULL, result, fieldobj);
    }

    return result;
}

Object_p ViewAsObj (View_p view) {
    View_p meta = V_Meta(view);
    int c, rows = ViewSize(view), cols = ViewSize(meta);
    Object_p result;
    struct Buffer buffer;
    
    InitBuffer(&buffer);

    if (meta == V_Meta(meta)) {
        if (rows > 0) {
            ADD_PTR_TO_BUF(buffer, Tcl_NewStringObj("mdef", 4));
            ADD_PTR_TO_BUF(buffer, MetaViewAsObj(view));
        }
    } else {
        if (cols == 0) {
            ADD_PTR_TO_BUF(buffer, Tcl_NewIntObj(rows));
        } else {
            ADD_PTR_TO_BUF(buffer, Tcl_NewStringObj("data", 4));
            ADD_PTR_TO_BUF(buffer, ViewAsObj(meta));
            for (c = 0; c < cols; ++c)
                ADD_PTR_TO_BUF(buffer, ColumnAsObj(ViewCol(view, c)));
        }
    }

    result = Tcl_NewObj();
    Tcl_InvalidateStringRep(result);
    
    SetupViewObj(result, view, BufferFill(&buffer) / sizeof(Object_p),
                                            BufferAsPtr(&buffer, 1), NULL);
    
    ReleaseBuffer(&buffer, 0);
    return result;
}

View_p ObjAsMetaView (Object_p obj) {
    int r, rows, objc;
    Object_p names, types, subvs, *objv, entry, nameobj, subvobj;
    const char *name, *sep, *type;
    View_p view = NULL;

    if (Tcl_ListObjLength(Interp(), obj, &rows) != TCL_OK)
        return NULL;

    names = Tcl_NewListObj(0, 0);
    types = Tcl_NewListObj(0, 0);
    subvs = Tcl_NewListObj(0, 0);

    for (r = 0; r < rows; ++r) {
        Tcl_ListObjIndex(NULL, obj, r, &entry);
        if (Tcl_ListObjGetElements(Interp(), entry, &objc, &objv) != TCL_OK ||
                objc < 1 || objc > 2)
            goto DONE;

        name = Tcl_GetString(objv[0]);
        sep = strchr(name, ':');
        type = objc > 1 ? "V" : "S";

        if (sep != NULL) {
            if (sep[1] != 0) {
                if (strchr("BDFLISV", sep[1]) == NULL)
                    goto DONE;
                type = sep+1;
            }
            nameobj= Tcl_NewStringObj(name, sep - name);
        } else
            nameobj = objv[0];

        if (objc > 1) {
            view = ObjAsMetaView(objv[1]);
            if (view == NULL)
                goto DONE;
            subvobj = ViewAsObj(view);
        } else
            subvobj = Tcl_NewObj();

        Tcl_ListObjAppendElement(NULL, names, nameobj);
        Tcl_ListObjAppendElement(NULL, types, Tcl_NewStringObj(type, -1));
        Tcl_ListObjAppendElement(NULL, subvs, subvobj);
    }

    view = NewView(V_Meta(EmptyMetaView()));
    SetViewCols(view, MC_name, 1, CoerceColumn(IT_string, names));
    SetViewCols(view, MC_type, 1, CoerceColumn(IT_string, types));
    SetViewCols(view, MC_subv, 1, CoerceColumn(IT_view, subvs));
    
DONE:
    DecObjRef(names);
    DecObjRef(types);
    DecObjRef(subvs);
    return view;
}

Object_p NeedMutable (Object_p obj) {
    int objc;
    Object_p *objv;
    View_p view;

    view = ObjAsView(obj);
    if (view == NULL)
        return NULL;
    
    if (IsMutable(view))
        return obj;
    
    objc = OBJ_TO_ORIG_CNT(obj);
    objv = OBJ_TO_ORIG_VEC(obj);
    
    if (objc == 1 && *Tcl_GetString(objv[0]) == '@') {
        const char *var = Tcl_GetString(objv[0]) + 1;
        Object_p o = (Object_p) Tcl_VarTraceInfo(Interp(), var,
                                            TCL_GLOBAL_ONLY, RefTracer, NULL);
        Assert(o != NULL);
        return o;
    }
    
    if (objc > 1 && strcmp(Tcl_GetString(objv[0]), "get") == 0) {
        Object_p origdef, tmp = NeedMutable(objv[1]); /* recursive */
        Assert(tmp != NULL);
        
        origdef = Tcl_NewListObj(objc, objv);
        Tcl_ListObjReplace(NULL, origdef, 1, 1, 1, &tmp);
        return origdef;
    }
    
    return obj;
}

static void ListOneDep (Buffer_p buf, Object_p obj) {
    Object_p o, vec[3];
    
    if (obj->typePtr == &f_viewObjType) {
        View_p view = OBJ_TO_VIEW_P(obj);
        Seq_p seq = ViewCol(view, 0).seq;
        vec[0] = Tcl_NewStringObj(seq != NULL ? seq->type->name : "", -1);
        vec[1] = MetaViewAsObj(V_Meta(view));
        vec[2] = Tcl_NewIntObj(ViewSize(view));
        o = Tcl_NewListObj(3, vec);
    } else if (Tcl_ListObjIndex(NULL, obj, 0, &o) != TCL_OK)
        o = Tcl_NewStringObj("?", 1);

    ADD_PTR_TO_BUF(*buf, o);
}

ItemTypes DepsCmd_O (Item_p a) {
    int i;
    Object_p obj, *objv;
    Seq_p origin;
    struct Buffer buffer;

    obj = a[0].o;
    ObjAsView(obj);
    
    if (obj->typePtr != &f_viewObjType)
        return IT_unknown;
    
    InitBuffer(&buffer);
    
    origin = OBJ_TO_ORIG_P(obj);
    objv = OBJ_TO_ORIG_VEC(obj);
    
    if (origin->OR_depx != NULL)
        ListOneDep(&buffer, origin->OR_depx);
    else
        ADD_PTR_TO_BUF(buffer, Tcl_NewStringObj("-", 1));
    
    for (i = 0; i < origin->OR_depc; ++i)
        ListOneDep(&buffer, objv[i]);
    
    a->o = BufferAsTclList(&buffer);
    return IT_object;
}
