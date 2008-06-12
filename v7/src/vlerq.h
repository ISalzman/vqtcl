/*  Vlerq public header.
    $Id$
    This file is part of Vlerq, see text at end for full copyright notice. */

#ifndef VLERQ_H
#define VLERQ_H

#include <stdint.h>
#include <lua.h>

#define VQ_VERSION      "1.7"
#define VQ_RELEASE      "1.7.0"
#define VQ_COPYRIGHT    "Copyright (C) 1996-2008 Jean-Claude Wippler"

LUA_API int luaopen_vq_core (lua_State *env);

/* vqType defines all types of data, usually passed around as vqSlot */
typedef enum { 
    VQ_nil, 
    VQ_int,
    VQ_pos, 
    VQ_long,
    VQ_float,
    VQ_double,
    VQ_string,
    VQ_bytes, 
    VQ_view
} vqType;

#define VQ_TYPES "NIPLFDSBV" /* canonical char code, indexed by vqType */

/* both vqView and vqColumn are opaque pointer types at this level */
typedef union vqView_u *vqView;
typedef union vqColumn_u *vqColumn;

/* a vqSlot can hold various data types, including some pairs */
typedef union vqSlot_u {
    int i;
    float f;
    const char *s;
    vqColumn c;
    vqView v;
    void *p;
    int64_t l;      /* careful, this overlaps the x.y fields on 32b machines */
    double d;       /* careful, this overlaps the x.y fields on 32b machines */
    struct {
        void *gap;
        union {
            int i;
            vqColumn c;
            vqView v;
            void *p;
        } y;
    } x;
} vqSlot;

/* memory management */

vqView (vq_incref) (vqView v);
void (vq_decref) (vqView v);

/* core view functions */

vqView (vq_new) (vqView m, int rows);
int (vq_isnil) (vqView v, int row, int col);
vqSlot (vq_get) (vqView v, int row, int col, vqType type, vqSlot def);
void (vq_set) (vqView v, int row, int col, vqType type, vqSlot val);
void (vq_replace) (vqView v, int start, int count, vqView data);

/* convenience wrappers */

int (vq_getInt) (vqView v, int row, int col, int def);
const char *(vq_getString) (vqView v, int row, int col, const char *def);
vqView (vq_getView) (vqView v, int row, int col, vqView def);
           
void (vq_setEmpty) (vqView v, int row, int col);
void (vq_setInt) (vqView v, int row, int col, int val);
void (vq_setString) (vqView v, int row, int col, const char *val);
void (vq_setView) (vqView v, int row, int col, vqView val);
void (vq_setMetaRow) (vqView v, int row, const char *, int, vqView);

/*  Copyright (C) 1996-2008 Jean-Claude Wippler.  All rights reserved.
  
    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:
  
    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.
  
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  
    [ MIT license: http://www.opensource.org/licenses/mit-license.php ] */

#endif
