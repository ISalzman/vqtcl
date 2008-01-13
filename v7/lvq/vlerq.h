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

LUA_API int luaopen_lvq_core (lua_State *L);

/* vqType defines all types of data, usually passed around as vqCell */
typedef enum { 
    VQ_nil, 
    VQ_int,
    VQ_long,
    VQ_float,
    VQ_double,
    VQ_string,
    VQ_bytes, 
    VQ_view
} vqType;

#define VQ_TYPES "NILFDSBV" /* canonical char code, when indexed by vqType */

/* a vqView is an opaque pointer at this level */
typedef struct vqView_s *vqView;

/* a vqCell can hold various data types, including some pairs */
typedef union vqCell_u {
    int i;
    int64_t l;
    float f;
    double d;
    const char *s;
    uint8_t *b;
    vqView v;
    void *p;
    int *n;
    struct {
        void *gap;
        union {
            int i;
            void *p;
        } y;
    } x;
} vqCell;

/* core view functions */

void (vq_init) (lua_State *L);
vqView (vq_new) (vqView m, int rows);
int (vq_isnil) (vqView v, int row, int col);
vqCell (vq_get) (vqView v, int row, int col, vqType type, vqCell def);
vqView (vq_set) (vqView v, int row, int col, vqType type, vqCell val);
vqView (vq_replace) (vqView v, int start, int count, vqView data);

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
