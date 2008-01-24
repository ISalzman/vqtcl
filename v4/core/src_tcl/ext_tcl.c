/*
 * ext_tcl.c - Interface to the Tcl scripting language.
 */

#include "ext_tcl.h"

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "4"
#endif

/* stub interface code, removes the need to link with libtclstub*.a */
#if STATIC_BUILD+0
#define MyInitStubs(x) 1
#else
#include "stubs.h"
#endif

#if NO_THREAD_CALLS+0
Shared_p GetShared (void) {
    static struct Shared data;
    return &data;
}
#define UngetShared Tcl_CreateExitHandler
#else
static Tcl_ThreadDataKey tls_data;
Shared_p GetShared (void) {
    return (Shared_p) Tcl_GetThreadData(&tls_data, sizeof(struct Shared));
}
#define UngetShared Tcl_CreateThreadExitHandler
#endif

typedef struct CmdDispatch {
    const char *name, *args;
    ItemTypes (*proc) (Item_p);
} CmdDispatch;

static CmdDispatch f_commands[] = {
#include "opdefs_gen.h"
    { NULL, NULL, NULL }
};

Object_p IncObjRef (Object_p objPtr) {
    Tcl_IncrRefCount(objPtr);
    return objPtr;
}

Object_p BufferAsTclList (Buffer_p bp) {
    int argc;
    Object_p result;
    
    argc = BufferFill(bp) / sizeof(Object_p);
    result = Tcl_NewListObj(argc, BufferAsPtr(bp, 1));
    ReleaseBuffer(bp, 0);
    return result;
}

void FailedAssert (const char *msg, const char *file, int line) {
    Tcl_Panic("Failed assertion at %s, line %d: %s\n", file, line, msg);
}

ItemTypes RefCmd_OX (Item args[]) {
    int objc;
    const Object_p *objv;
    
    objv = (const void*) args[1].u.ptr;
    objc = args[1].u.len;

    args->o = Tcl_ObjGetVar2(Interp(), args[0].o, 0,
                                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    return IT_object;
}

ItemTypes OpenCmd_S (Item args[]) {
    View_p result;
    
    result = OpenDataFile(args[0].s);
    if (result == NULL) {
        Tcl_AppendResult(Interp(), "cannot open file: ", args[0].s, NULL);
        return IT_unknown;
    }
    
    args->v = result;
    return IT_view;
}

static void LoadedStringCleaner (MappedFile_p map) {
    DecObjRef((Object_p) map->data[3].p);
}

ItemTypes LoadCmd_O (Item args[]) {
    int len;
    const char *ptr;
    View_p view;
    MappedFile_p map;
    
    ptr = (const char*) Tcl_GetByteArrayFromObj(args[0].o, &len);

    map = InitMappedFile(ptr, len, LoadedStringCleaner);
    map->data[3].p = IncObjRef(args[0].o);
    
    view = MappedToView(map, NULL);
    if (view == NULL)
        return IT_unknown;
        
    args->v = view;
    return IT_view;
}

ItemTypes LoadModsCmd_OV (Item args[]) {
    int len;
    const char *ptr;
    View_p view;
    MappedFile_p map;
    
    ptr = (const char*) Tcl_GetByteArrayFromObj(args[0].o, &len);

    map = InitMappedFile(ptr, len, LoadedStringCleaner);
    map->data[3].p = IncObjRef(args[0].o);
    
    view = MappedToView(map, args[1].v);
    if (view == NULL)
        return IT_unknown;
        
    args->v = view;
    return IT_view;
}

    static void *WriteDataFun(void *chan, const void *ptr, Int_t len) {
        Tcl_Write(chan, ptr, len);
        return chan;
    }

ItemTypes SaveCmd_VS (Item args[]) {
    Int_t bytes;
    Tcl_Channel chan;
    
    chan = Tcl_OpenFileChannel(Interp(), args[1].s, "w", 0666);
    if (chan == NULL || Tcl_SetChannelOption(Interp(), chan,
                                            "-translation", "binary") != TCL_OK)
        return IT_unknown;
        
    bytes = ViewSave(args[0].v, chan, NULL, WriteDataFun, 0);

    Tcl_Close(Interp(), chan);

    if (bytes < 0)
        return IT_unknown;
        
    args->w = bytes;
    return IT_wide;
}

ItemTypes WriteCmd_VO (Item args[]) {
    Int_t bytes;
    Tcl_Channel chan;
    
    chan = Tcl_GetChannel(Interp(), Tcl_GetString(args[1].o), NULL);
    if (chan == NULL || Tcl_SetChannelOption(Interp(), chan,
                                            "-translation", "binary") != TCL_OK)
        return IT_unknown;
        
    bytes = ViewSave(args[0].v, chan, NULL, WriteDataFun, 0);
    
    if (bytes < 0)
        return IT_unknown;
        
    args->w = bytes;
    return IT_wide;
}

#define EmitInitFun ((SaveInitFun) Tcl_SetByteArrayLength)

    static void *EmitDataFun(void *data, const void *ptr, Int_t len) {
        memcpy(data, ptr, len);
        return (char*) data + len;
    }

ItemTypes EmitCmd_V (Item args[]) {
    Object_p result = Tcl_NewByteArrayObj(NULL, 0);
        
    if (ViewSave(args[0].v, result, EmitInitFun, EmitDataFun, 0) < 0) {
        DecObjRef(result);
        return IT_unknown;
    }
        
    args->o = result;
    return IT_object;
}

ItemTypes EmitModsCmd_V (Item args[]) {
    Object_p result = Tcl_NewByteArrayObj(NULL, 0);
        
    if (ViewSave(args[0].v, result, EmitInitFun, EmitDataFun, 1) < 0) {
        DecObjRef(result);
        return IT_unknown;
    }
        
    args->o = result;
    return IT_object;
}

int AdjustCmdDef (Object_p cmd) {
    Object_p origname, newname;
    Tcl_CmdInfo cmdinfo;

    /* Use "::vlerq::blah ..." if it exists, else use "vlerq blah ...". */
    /* Could perhaps be simplified (optimized?) by using 8.5 ensembles. */
     
    if (Tcl_ListObjIndex(Interp(), cmd, 0, &origname) != TCL_OK)
        return TCL_ERROR;

    /* insert "::vlerq::" before the first list element */
    newname = Tcl_NewStringObj("::vlerq::", -1);
    Tcl_AppendObjToObj(newname, origname);
    
    if (Tcl_GetCommandInfo(Interp(), Tcl_GetString(newname), &cmdinfo))
        Tcl_ListObjReplace(NULL, cmd, 0, 1, 1, &newname);
    else {
        Object_p buf[2];
        DecObjRef(newname);
        buf[0] = Tcl_NewStringObj("vlerq", -1);
        buf[1] = origname;
        Tcl_ListObjReplace(NULL, cmd, 0, 1, 2, buf);
    }
    return TCL_OK;
}

ItemTypes ViewCmd_X (Item args[]) {
    int e = TCL_OK, i, n, index, objc;
    const Object_p *objv;
    Object_p result, buf[10], *cvec;
    Tcl_Interp *interp = Interp();
    const CmdDispatch *cmds = GetShared()->info->cmds;

    objv = (const void*) args[0].u.ptr;
    objc = args[0].u.len;

    if (objc < 1) {
        Tcl_WrongNumArgs(interp, 0, objv, "view arg ?op ...? ?| ...?");
        return IT_unknown;
    }
    
    Tcl_SetObjResult(interp, objv[0]); --objc; ++objv;
    
    while (e == TCL_OK && objc > 0) {
        for (n = 0; n < objc; ++n)
            if (objv[n]->bytes != 0 && *objv[n]->bytes == '|' && 
                    objv[n]->length == 1)
                break;

        if (n > 0) {
            cvec = n > 8 ? malloc((n+2) * sizeof(Object_p)) : buf;
                
            if (Tcl_GetIndexFromObjStruct(NULL, *objv, cmds, sizeof *cmds, 
                                            "", TCL_EXACT, &index) != TCL_OK)
                index = -1;

            cvec[0] = Tcl_NewStringObj("vlerq", -1);
            cvec[1] = objv[0];
            cvec[2] = IncObjRef(Tcl_GetObjResult(interp));
            for (i = 1; i < n; ++i)
                cvec[i+2] = objv[i];
            
            result = Tcl_NewListObj(n+1, cvec+1);

            if (index < 0 || *cmds[index].args != 'V') {
                e = AdjustCmdDef(result);
                if (e == TCL_OK) {
                    int ac;
                    Object_p *av;
                    Tcl_ListObjGetElements(NULL, result, &ac, &av);
                    /* don't use Tcl_EvalObjEx, it forces a string conversion */
                    e = Tcl_EvalObjv(interp, ac, av, 0);
                }
                DecObjRef(result);
            } else
                Tcl_SetObjResult(interp, result);
            
            DecObjRef(cvec[2]);
            if (n > 8)
                free(cvec);
        }
        
        objc -= n+1; objv += n+1; /*++k;*/
    }

    if (e != TCL_OK) {
#if 0
        char msg[50];
        sprintf(msg, "\n        (\"view\" step %d)", k);
        Tcl_AddObjErrorInfo(interp, msg, -1);
#endif
        return IT_unknown;
    }
    
    args->o = Tcl_GetObjResult(interp);
    return IT_object;
}

#define MAX_STACK 20

static int VlerqObjCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    int i, index, ok = TCL_ERROR;
    ItemTypes type;
    Object_p result;
    Item stack [MAX_STACK];
    const char *args;
    const CmdDispatch *cmdtable;
    PUSH_KEEP_REFS
    
    Interp() = interp;
    
    if (objc <= 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "command ...");
        goto FAIL;
    }

    cmdtable = (const CmdDispatch*) data;
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], cmdtable, sizeof *cmdtable, 
                                    "command", TCL_EXACT, &index) != TCL_OK)
        goto FAIL;

    objv += 2; objc -= 2;
    args = cmdtable[index].args + 2; /* skip return type and ':' */

    for (i = 0; args[i] != 0; ++i) {
        Assert(i < MAX_STACK);
        if (args[i] == 'X') {
            Assert(args[i+1] == 0);
            stack[i].u.ptr = (const void*) (objv+i);
            stack[i].u.len = objc-i;
            break;
        }
        if ((args[i] == 0 && i != objc) || (args[i] != 0 && i >= objc)) {
            char buf [10*MAX_STACK];
            const char* s;
            *buf = 0;
            for (i = 0; args[i] != 0; ++i) {
                if (*buf != 0)
                    strcat(buf, " ");
                switch (args[i] & ~0x20) {
                    case 'C': s = "list"; break;
                    case 'I': s = "int"; break;
                    case 'N': s = "col"; break;
                    case 'O': s = "any"; break;
                    case 'S': s = "string"; break;
                    case 'V': s = "view"; break;
                    case 'X': s = "..."; break;
                    default: Assert(0); s = "?"; break;
                }
                strcat(buf, s);
                if (args[i] & 0x20)
                    strcat(buf, "*");
            }
            Tcl_WrongNumArgs(interp, 2, objv-2, buf);
            goto FAIL;
        }
        stack[i].o = objv[i];
        if (!CastObjToItem(args[i], stack+i)) {
            if (*Tcl_GetStringResult(interp) == 0) {
                const char* s = "?";
                switch (args[i] & ~0x20) {
                    case 'C': s = "list"; break;
                    case 'I': s = "integer"; break;
                    case 'N': s = "column name"; break;
                    case 'V': s = "view"; break;
                }
                Tcl_AppendResult(interp, cmdtable[index].name, ": invalid ", s,
                                                                        NULL);
            }
            goto FAIL; /* TODO: append info about which arg is bad */
        }
    }
    
    GetShared()->info->cmds = cmdtable; /* for ViewCmd_X */
    
    type = cmdtable[index].proc(stack);
    if (type == IT_unknown)
        goto FAIL;

    result = ItemAsObj(type, stack);
    if (result == NULL)
        goto FAIL;

    Tcl_SetObjResult(interp, result);
    ok = TCL_OK;

FAIL:
    POP_KEEP_REFS
    return ok;
}

static void DoneShared (ClientData cd) {
    Shared_p sh = (Shared_p) cd;
    if (--sh->refs == 0)
        free(sh->info);
}

static void InitShared (void) {
    Shared_p sh = GetShared();
    if (sh->refs++ == 0)
        sh->info = calloc(1, sizeof(SharedInfo));
    UngetShared(DoneShared, (ClientData) sh);
}

DLLEXPORT int Vlerq_Init (Tcl_Interp *interp) {
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    Tcl_CreateObjCommand(interp, "vlerq", VlerqObjCmd, f_commands, NULL);
    InitShared();
    return Tcl_PkgProvide(interp, "vlerq", PACKAGE_VERSION);
}

DLLEXPORT int Vlerq_SafeInit (Tcl_Interp *interp) {
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    /* UNSAFE is defined in the "opdefs_gen.h" file included above */
    Tcl_CreateObjCommand(interp, "vlerq", VlerqObjCmd, f_commands + UNSAFE,
                                                                        NULL);
    InitShared();
    return Tcl_PkgProvide(interp, "vlerq", PACKAGE_VERSION);
}
