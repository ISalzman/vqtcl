/*
 * loop.c - Implementation of the Tcl-specific "loop" operator.
 */

/* TODO: code was copied from v3, needs a lot of cleanup */

#include "ext_tcl.h"

    typedef struct LoopInfo {
        int row;
        Object_p view;
        View_p v;
        Object_p cursor;
    } LoopInfo;

    typedef struct ColTrace {
        LoopInfo* lip;
        int col;
        Object_p name;
        int lastRow;
    } ColTrace;

static char* cursor_tracer(ClientData cd, Tcl_Interp* ip, const char *name1, const char *name2, int flags) {
    ColTrace* ct = (ColTrace*) cd;
    int row = ct->lip->row;
    if (row != ct->lastRow) {
        Object_p o;
        if (ct->col < 0)
            o = Tcl_NewIntObj(row);
        else {
            Item item;
            item.c = ViewCol(ct->lip->v, ct->col);
            o = ItemAsObj(GetItem(row, &item), &item);
        }
        if (Tcl_ObjSetVar2(ip, ct->lip->cursor, ct->name, o, 0) == 0)
            return "cursor_tracer?";
        ct->lastRow = row;
    }
    return 0;
}

static int loop_vop(int oc, Tcl_Obj* const* ov) {
    LoopInfo li;
    const char *cur;
    struct Buffer result;
    int type, e = TCL_OK;
    static const char *options[] = { "-where", "-index", "-collect", 0 };
    enum { eLOOP = -1, eWHERE, eINDEX, eCOLLECT };

    if (oc < 2 || oc > 4) {
        Tcl_WrongNumArgs(Interp(), 1, ov, "view ?arrayName? ?-type? body");
        return TCL_ERROR;
    }

    li.v = ObjAsView(ov[0]);
    if (li.v == NULL)
        return TCL_ERROR;

    --oc; ++ov;
    if (oc > 1 && *Tcl_GetString(*ov) != '-') {
        li.cursor = *ov++; --oc;
    } else
        li.cursor = Tcl_NewObj();
    cur = Tcl_GetString(IncObjRef(li.cursor));

    if (Tcl_GetIndexFromObj(0, *ov, options, "", 0, &type) == TCL_OK) {
        --oc; ++ov;
    } else
        type = eLOOP;

    {
        int i, nr = ViewSize(li.v), nc = ViewWidth(li.v);
        ColTrace* ctab = (ColTrace*) ckalloc((nc + 1) * sizeof(ColTrace));
        InitBuffer(&result);
        for (i = 0; i <= nc; ++i) {
            ColTrace* ctp = ctab + i;
            ctp->lip = &li;
            ctp->lastRow = -1;
            if (i < nc) {
                ctp->col = i;
                ctp->name = Tcl_NewStringObj(GetColItem(i,
                              ViewCol(V_Meta(li.v), MC_name), IT_string).s, -1);
            } else {
                ctp->col = -1;
                ctp->name = Tcl_NewStringObj("#", -1);
            }
            e = Tcl_TraceVar2(Interp(), cur, Tcl_GetString(ctp->name),
                              TCL_TRACE_READS, cursor_tracer, (ClientData) ctp);
        }
        if (e == TCL_OK)
            switch (type) {
                case eLOOP:
                    for (li.row = 0; li.row < nr; ++li.row) {
                        e = Tcl_EvalObj(Interp(), *ov);
                        if (e == TCL_CONTINUE)
                            e = TCL_OK;
                        else if (e != TCL_OK) {
                            if (e == TCL_BREAK)
                                e = TCL_OK;
                            else if (e == TCL_ERROR) {
                                char msg[50];
                                sprintf(msg, "\n    (\"loop\" body line %d)",
                                                        Interp()->errorLine);
                                Tcl_AddObjErrorInfo(Interp(), msg, -1);
                            }
                            break;
                        }
                    }
                    break;
                case eWHERE:
                case eINDEX:
                    for (li.row = 0; li.row < nr; ++li.row) {
                        int f;
                        e = Tcl_ExprBooleanObj(Interp(), *ov, &f);
                        if (e != TCL_OK)
                            break;
                        if (f)
                            ADD_INT_TO_BUF(result, li.row);
                    }
                    break;
                case eCOLLECT:
                    for (li.row = 0; li.row < nr; ++li.row) {
                        Object_p o;
                        e = Tcl_ExprObj(Interp(), *ov, &o);
                        if (e != TCL_OK)
                            break;
                        ADD_PTR_TO_BUF(result, o);
                    }
                    break;
                default: Assert(0); return TCL_ERROR;
            }
        for (i = 0; i <= nc; ++i) {
            ColTrace* ctp = ctab + i;
            Tcl_UntraceVar2(Interp(), cur, Tcl_GetString(ctp->name),
                            TCL_TRACE_READS, cursor_tracer, (ClientData) ctp);
            DecObjRef(ctp->name);
        }
        ckfree((char*) ctab);
        if (e == TCL_OK)
            switch (type) {
                case eWHERE: case eINDEX: {
                    Column map = SeqAsCol(BufferAsIntVec(&result));
                    if (type == eWHERE)
                        Tcl_SetObjResult(Interp(), 
                                ViewAsObj(RemapSubview(li.v, map, 0, -1)));
                    else
                        Tcl_SetObjResult(Interp(), ColumnAsObj(map));
                    break;
                }
                case eCOLLECT: {
                    Tcl_SetObjResult(Interp(), BufferAsTclList(&result));
                    break;
                }
                default:
                    ReleaseBuffer(&result, 0);
            }
    }
    
    DecObjRef(li.cursor);
    return e;
}

ItemTypes LoopCmd_X (Item args[]) {
    int objc;
    const Object_p *objv;

    objv = (const void*) args[0].u.ptr;
    objc = args[0].u.len;

    if (loop_vop(objc, objv) != TCL_OK)
        return IT_unknown;
        
    args->o = Tcl_GetObjResult(Interp());
    return IT_object;
}
