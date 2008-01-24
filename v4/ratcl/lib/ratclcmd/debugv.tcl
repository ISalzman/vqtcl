ratcl::vopdef debugv {w cmds} {
    upvar ::ratcl::_debugv v
    set v $w
    if {[catch {view $v size}]} {
        set results ""
    } else {
        set results [list $v ""] 
    }
    set x ""
    foreach c [split $cmds \n] {
      append x $c \n
      if {![info complete $x]} continue
      set x [string trim $x]
      if {[regexp {^#} $x]} {
        lappend results [view 0] $x
        set x ""
      } elseif {[string length $x] > 0} {
        set cm "view \$::ratcl::_debugv $x"
        set v [uplevel 1 $cm]
        catch {view $v size}  ;# Force application of lazy ops
        lappend results $v $x
      }
      set x ""
    }
    unset v
    return [view {view:V command} def $results] 
  }
