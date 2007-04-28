// xvq.c -- Fifth-generation experimental version of the Vlerq core engine
// Copyright 2007 Jean-Claude Wippler <jcw@equi4.com>.  All rights reserved.
//
// The object design is modeled after id-objmodel/obj.c by Ian Piumarta.

#if 0
    TODO: add gc, see vlerq2/src/thrive.h
#endif

#define USE_TCL_STUBS 1
#include <tcl.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// --------------------------------------------------------- DATA STRUCTURES ---

typedef            long   Int;
typedef   struct Header * Header;
typedef struct Behavior * Behavior;
typedef   struct Object * Object;
typedef   struct String * String;
typedef     struct Cell * CellVec;

typedef struct Cell { Object p; Int i; } Cell;

typedef Cell (*Method)(Object,Object,...);

#define CELLS(type) (sizeof(struct type) / sizeof(Cell))

struct Header {
           Object  chain;
              Int  ncells;
         Behavior  behavior;
              Int  bytes;
};

struct Behavior {
    struct Header  head[0];
          CellVec  keys;
              Int  fill;
         Behavior  parent;
              int  _dummy;
};

struct Object {
    struct Header  head[0];
};

struct String {
    struct Header  head[0];
             char  data[0];
};

static String s_addMethod, s_allocate, s_delegated, s_lookup;

static Behavior object_beh, vtable_beh;

static Behavior symbol_table;

// ----------------------------------------------------------- OBJECT SYSTEM ---

#define SEND(rcv,msg,args...) ({ \
        Object r = (Object) rcv; \
        Cell c = bindMsg(r, msg); \
        ((Method) c.i)(c.p, r, ##args); \
    })

/*
#define makeCell(ptr,idx) ((Cell) { .p = (ptr), .i = (idx) })
*/

static Cell makeCell (void* object, Int index) {
    return ((Cell) { object, index });
}

static Object newObject (int cells, int bytes) {
    Header h = (Header) calloc(1, (CELLS(Header)+cells) * sizeof(Cell) + bytes);
    CellVec v = (CellVec) (h+1);
    for (int i = -CELLS(Header); i < cells; ++i)
        v[i].p = (Object) symbol_table;
    if (symbol_table != 0) {
        h->chain = (Object) symbol_table;
        symbol_table->head[-1].chain = (Object) v;
    }
    h->ncells = cells;
    h->bytes = bytes;
    h->behavior = object_beh;
    return (Object) v;
}

static Object newAllocate (Object data, Behavior self, int ncells, int bytes) {
    Object o = newObject(ncells, bytes);
    o->head[-1].behavior = self;
    return o;
}

static String newString (const char* string) {
    int length = strlen(string) + 1;
    return memcpy(newObject(0, length), string, length);
}

struct Cell rawLookup (Object data, Behavior self, String key)
{
    for (int i = 0; i < self->fill; ++i)
        if ((Object) key == self->keys[i].p) {
            int limit = ((Object) self->keys)->head[-1].ncells / 2;
            return self->keys[i+limit];
        }
    fprintf(stderr, "lookup failed %p %s\n", self, key->data);
    return makeCell(0, 0);
}

static Cell bindMsg (Object rcv, String msg) {
    Behavior b = rcv->head[-1].behavior;
    Cell c = msg == s_lookup && rcv == (Object) vtable_beh ?
                rawLookup(0, b, msg) : SEND(b, s_lookup, msg);
    return c;
}

static Method addMethod (Object data, Behavior self, String key, Method method) {
    int limit = ((Object) self->keys)->head[-1].ncells / 2;
    for (int i = 0; i < self->fill; ++i)
        if ((Object) key == self->keys[i].p) {
            self->keys[i+limit].i = (Int) method;
            return method;
        }
    if (self->fill == limit) {
        CellVec newkeys = (CellVec) newObject(4 * limit, 0);
        memcpy(newkeys, self->keys, limit * sizeof(Cell));
        memcpy(newkeys + 2 * limit, self->keys + limit, limit * sizeof(Cell));
        self->keys = newkeys;
        limit *= 2;
    }
    self->keys[self->fill].p = (Object) key;
    self->keys[self->fill++ + limit].i = (Int) method;
    return method;
}

static String asSymbol (Object data, Object self, const char* string) {
    for (int i = 0; i < symbol_table->fill; ++i) {
        String s = (String) symbol_table->keys[i].p;
        if (strcmp(string, s->data) == 0)
            return s;
    }
    String t = newString(string);
    addMethod(0, symbol_table, t, 0);
    return t;
}

static Behavior newDelegated (Object data, Behavior self) {
    Behavior child = (Behavior) newObject(CELLS(Behavior), 0);
    child->head[-1].behavior = self != 0 ? self->head[-1].behavior : 0;
    child->keys = (CellVec) newObject(4, 0);
    child->parent = self;
    return child;
}

static void initObjects (void) {
    vtable_beh = newDelegated(0, 0);
    vtable_beh->head[-1].behavior = vtable_beh;

    object_beh = newDelegated(0, 0);
    object_beh->head[-1].behavior = vtable_beh;
    vtable_beh->parent = object_beh;

    symbol_table = newDelegated(0, object_beh);
    
    symbol_table->head[-1].chain = (Object) object_beh;
    object_beh->head[-1].chain = (Object) vtable_beh;
    vtable_beh->head[-1].chain = (Object) symbol_table;

    s_lookup	= asSymbol(0, 0, "lookup");
    s_addMethod = asSymbol(0, 0, "addMethod");
    s_allocate  = asSymbol(0, 0, "allocate");
    s_delegated = asSymbol(0, 0, "delegated");

    addMethod(0, vtable_beh, s_lookup, (Method) rawLookup);
    addMethod(0, vtable_beh, s_addMethod, (Method) addMethod);

    SEND(vtable_beh, s_addMethod, s_allocate, newAllocate);
    SEND(vtable_beh, s_addMethod, s_delegated, newDelegated);  
}

// ------------------------------------------------------- OBJECT EXTENSIONS ---

#if 0
typedef    struct Table * Table;

struct Table {
    struct Header  head[0];
            Table  meta;
              Int  size;
             Cell  columns[];
};

static Table newTable (Table meta) {
    Table t = newObject(meta->size + 1, 0);
    t->meta = meta;
    return t;
}

static Cell rawGetCell (CellVec vector, int index) {
    assert(-CELLS(Header) <= index && index < vector->head[-1].ncells);
    return vector[index];
}

static void rawSetCell (CellVec vector, int index, Cell value) {
    assert(0 <= index && index < vector->head[-1].ncells);
    vector[index] = value;
}
#endif

// ----------------------------------------------------------- TCL INTERFACE ---

static int xvqCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    Tcl_SetResult(interp, "ha!", TCL_STATIC);
    return TCL_OK;
}

DLLEXPORT int Xvq_Init (Tcl_Interp *interp) {
    Tcl_InitStubs(interp, "8.4", 0);
    initObjects();
    Tcl_CreateObjCommand(interp, "xvq", xvqCmd, 0, 0);
    return Tcl_PkgProvide(interp, "xvq", "0.1");
}
