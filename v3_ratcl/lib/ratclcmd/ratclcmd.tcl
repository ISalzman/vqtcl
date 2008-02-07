package require ratcl
set cmd ratcl
foreach file [glob -dir [file dirname [info script]] *.tcl] {
    switch -glob $file [info script] - *pkgIndex.tcl {} default {
        source $file
        lappend cmd [file tail [file rootname $file]]
    }
}
if {[info exists tkcon]} {
    puts stderr "loaded [join $cmd {, }]"
}
package provide ratclcmd 0.1