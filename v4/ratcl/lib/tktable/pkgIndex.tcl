package ifneeded Tktable 2.9  [string map [list #dir [list $dir]] {
  package require Tk
  if {$::tcl_platform(os) eq "Darwin" && [tk windowingsystem] eq "x11"} {
    load [file join #dir libTktable2.9.X11[info shared]] Tktable
  } else {
    load [file join #dir libTktable2.9[info shared]] Tktable
  }
}]
