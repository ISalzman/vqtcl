/* V9 test code, not needed for production use
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"

const char* V9_AllocName (V9View v) {
    return Head(v).alloc->name;
}

const char* V9_TypeName (V9View v) {
    return Head(v).type->name;
}
