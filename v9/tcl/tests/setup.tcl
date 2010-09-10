# V9 test setup, sourced by all test scripts
# 2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
# $Id$

package require Tcl 8.5

if {[info commands test] eq ""} {
    package require tcltest
    namespace import tcltest::*
}

configure {*}$argv

testsDirectory [file dirname [info script]]
workingDirectory [file dirname [testsDirectory]]

# TextMate support on Mac OS X: run make before running any test from editor
if {[info exists env(TM_FILENAME)]} {
    if {[catch { exec make } msg]} {
        puts stderr $msg
        exit 1
    }
}

# make sure the pkgIndex.tcl is found
if {[lsearch $auto_path [workingDirectory]] < 0} {
    lappend auto_path [workingDirectory]
}

# extract version number from pkgIndex.tcl so tests don't have to hardwire it
regexp {ifneeded V9\s(\S+)\s} [viewFile pkgIndex.tcl] version version

# convenience definition for cleanup at end of all tests
proc cleanup {{vars {}} {cmds {}}} {
    test cleanup-1 {clean up temporary variables} {
        uplevel unset {*}$vars
        uplevel info vars ?
    } ""
    test cleanup-2 {clean up temporary commands} {
        foreach x $cmds {
            rename $x ""
        }
        info commands ?
    } ""
    cleanupTests
}
