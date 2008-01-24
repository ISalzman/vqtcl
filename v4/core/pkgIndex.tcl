if {[file exists [file join $dir libvlerq4[info sharedlibext]]]} {
  package ifneeded vlerq 4 \
    [list load [file join $dir libvlerq4[info sharedlibext]]]
}

if {[file exists [file join $dir src_tcl ratcl.tcl]]} {
  package ifneeded ratcl 4 \
    [list source [file join $dir src_tcl ratcl.tcl]]
}
