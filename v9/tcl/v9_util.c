/* V9 utility code
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"

V9View V9_AddRef (V9View v) {
	if (v == 0)
		v = EmptyMeta();
	return (V9View) IncRef((Vector) v);
}

V9View V9_Release (V9View v) {
	return (V9View) DecRef((Vector) v);
}

V9View V9_Meta (V9View v) {
	if (v == 0)
		v = EmptyMeta();
	return v->meta;
}

int V9_Size (V9View v) {
	return Head(v).count;
}

int V9_GetInt (V9View v, int row, int col) {
    return V9_Get(v, row, col, 0).i;
}

const char* V9_GetString (V9View v, int row, int col) {
    return V9_Get(v, row, col, 0).s;
}

V9View V9_GetView (V9View v, int row, int col) {
    return V9_Get(v, row, col, 0).v;
}

V9Types V9_CharAsType (char type) {
	switch (type) {
		case 'I':   return V9T_int;
		case 'L':   return V9T_long;
		case 'F':   return V9T_float;
		case 'D':   return V9T_double;
		case 'S':   return V9T_string;
		case 'B':   return V9T_bytes;
		case 'V':   return V9T_view;
	}
	return V9T_nil;
}
