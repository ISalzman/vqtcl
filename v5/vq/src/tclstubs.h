/* Internal stub code, adapted from CritLib */

TclStubs               *tclStubsPtr        = NULL;
TclPlatStubs           *tclPlatStubsPtr    = NULL;
struct TclIntStubs     *tclIntStubsPtr     = NULL;
struct TclIntPlatStubs *tclIntPlatStubsPtr = NULL;

static int MyInitStubs (Tcl_Interp *ip) {

    typedef struct HeadOfInterp {
        char         *result;
        Tcl_FreeProc *freeProc;
        int           errorLine;
        TclStubs     *stubTable;
    } HeadOfInterp;

    HeadOfInterp *hoi = (HeadOfInterp*) ip;

    if (hoi->stubTable == NULL || hoi->stubTable->magic != TCL_STUB_MAGIC) {
        ip->result = "This extension requires stubs-support.";
        ip->freeProc = TCL_STATIC;
        return 0;
    }

    tclStubsPtr = hoi->stubTable;

    if (Tcl_PkgRequire(ip, "Tcl", "8.1", 0) == NULL) {
        tclStubsPtr = NULL;
        return 0;
    }

    if (tclStubsPtr->hooks != NULL) {
        tclPlatStubsPtr = tclStubsPtr->hooks->tclPlatStubs;
        tclIntStubsPtr = tclStubsPtr->hooks->tclIntStubs;
        tclIntPlatStubsPtr = tclStubsPtr->hooks->tclIntPlatStubs;
    }

    return 1;
}
