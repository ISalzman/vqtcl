# thrive.tcl -=- wraps thrive.c from the Tcl extension, using critcl

package require critcl
package provide thrive 2.23

critcl::cheaders -Ithrive/generic

critcl::ccode {
#define CRITCL 1
#include "thrive.c"
}

namespace eval thrive {
  critcl::ccommand new {cd ip oc ov} { return new_Cmd(cd,ip,oc,ov); }
  critcl::ccommand isref {cd ip oc ov} { return isref_Cmd(cd,ip,oc,ov); }
}
