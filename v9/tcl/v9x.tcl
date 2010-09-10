# V9 Tcl script code to load the V9x extension
# 2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
# $Id$

package require V9

load [file join [file dirname [info script]] $v9(libfile)] V9x

package require -exact V9 $v9(version)  ;# verify that proper v9.tcl is loaded
package require -exact V9x $v9(version) ;# verify proper C extension is loaded

# the definitions below make assumptions which require V9x to be loaded

namespace eval v9 {
        
    proc iota {n} {
        return [list step $n 0 1 1]
    }
    
    proc first {v n} {
        return [list rowmap $v [iota $n]]
    }
    
    proc spread {v n} {
        return [list rowmap $v [list step [expr {$n * [size $v]}] 0 1 $n]]
    }
    
    proc product {v w} {
        return [list pair [spread $v [size $w]] [list times $w [size $v]]]
    }
    
    proc tag {v} {
        return [list pair $v [iota [size $v]]]
    }
    
    proc sort {v} {
        return [list rowmap $v [list sortmap $v]]
    }
    
    proc unique {v} {
        return [list rowmap $v [list uniqmap $v]]
    }
    
    proc except {v w} {
        return [list rowmap $v [list exceptmap $v $w]]
    }
    
    proc intersect {v w} {
        return [list rowmap $v [list isectmap $v $w]]
    }

    proc union {v w} {
        return [list plus $v [except $w $v]]
    }
}
