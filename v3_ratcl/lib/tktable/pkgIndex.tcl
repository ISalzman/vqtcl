if {[catch {package require Tcl 8.2}]} return
if {[catch {package require Tk 8.2}]} return
set ext [info shared]
# this is a little inefficient - we should do this in the "ifneeded" rather
# than when the pkgIndex.tcl is read
if {$::tcl_platform(os) eq "Darwin" && [tk windowingsystem] eq "x11"} {
    set ext .X11$ext
}
package ifneeded Tktable 2.9  [list load [file join $dir libTktable2.9$ext] Tktable]
