# common test setup script

if {[info exists testsInited]} return

package require Tcl 8.4

if {[lsearch [namespace children] ::tcltest] == -1} {
    package require tcltest 2.2
    namespace import tcltest::*
}

singleProcess true ;# run without forking

testsDirectory [file dirname [info script]]

if {![file exists pkgIndex.tcl]} {
    cd [testsDirectory]/../build
}

temporaryDirectory [pwd]
workingDirectory [pwd]

# TextMate support on Mac OS X: run make before running any test from editor
if {[info exists env(TM_FILENAME)]} {
    if {[catch { exec make } msg]} {
        puts stderr $msg
        exit 1
    }
}

proc readfile {filename {mode ""}} {                                                      
    set fd [open $filename]      
    if {$mode eq "-binary"} {
        fconfigure $fd -translation binary
    }                                                 
    set data [read $fd]                                                           
    close $fd                                                                     
    return $data                                                                  
}

# extract version number from pkgIndex.tcl
regexp {ifneeded vq\s(\S+)\s} [readfile pkgIndex.tcl] - version
unset -

# make sure the pkgIndex.tcl is found
if {[lsearch $auto_path [workingDirectory]] < 0} {
    lappend auto_path [workingDirectory]
}

testConstraint 32bit            [expr {$tcl_platform(wordSize) == 4}]
testConstraint 64bit            [expr {$tcl_platform(wordSize) == 8}]
testConstraint bigendian        [expr {$tcl_platform(byteOrder) eq "bigEndian"}]
testConstraint kitten           [file exists [temporaryDirectory]/kitten.kit]
testConstraint metakit          [expr {![catch { package require Mk4tcl }]}]
testConstraint tcl$tcl_version  1
testConstraint vfs              [expr {![catch { package require vfs }]}]

set testsInited 1

# table pretty-printer, using a minimal set of operators: at, empty, meta, size

proc vdump {vid {maxrows 20}} {
    set rows [vq size $vid]
    set cols [vq size [vq meta $vid]]
    if {$cols == 0} { return "  ($rows rows, no columns)" }
    if {$rows > $maxrows} { set rows $maxrows }
    for {set c 0} {$c < $cols} {incr c} {
        set name [vq at [vq meta $vid] $c 0 ""]
        set type [string index "NILFDSBTO" [vq at [vq meta $vid] $c 1 0]]
        set data {}
        for {set r 0} {$r < $rows} {incr r} {
            if {[vq empty $vid $r $c]} {
                lappend data ?
            } else {
                set x [vq at $vid $r $c 0]
                switch $type {
                    B       { lappend data [string length $x]b }
                    T       { lappend data #[vq size $x] }
                    default { lappend data $x }
                }
            }
        }
        set width [string length $name]
        foreach z $data {
            if {[string length $z] > $width} { set width [string length $z] }
        }
        if {$width > 50} { set width 50 }
        switch $type {
            B - I - L - F - D - T   { append fmt "  " %${width}s }
            default                 { append fmt "  " %-$width.${width}s }
        }
        append hdr "  " [format %-${width}s $name]
        append bar "  " [string repeat - $width]
        lappend d $data
    }
    set r [list $hdr $bar]
    for {set i 0} {$i < $rows} {incr i} {
        if {$i >= $maxrows} break
        set cmd [list format $fmt]
        foreach x $d { lappend cmd [regsub -all {[^ -~]} [lindex $x $i] .] }
        lappend r [eval $cmd]
    }
    if {[vq size $vid] > $maxrows} { lappend r [string map {- .} $bar] }
    ::join $r \n
}
