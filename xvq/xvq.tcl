# xvq.tcl -- fifth-generation experimental version of the vlerq core engine
# Copyright 2007 Jean-Claude Wippler <jcw@equi4.com>.  All rights reserved.

# %renumber<^\s*test >%

package require tcltest 2.2
namespace import tcltest::test

# TextMate support on Mac OS X: run make before running any test from editor
if {[info exists env(TM_FILENAME)]} { exec make ;#>@stdout }

test 0 {load extension} {
    load ./xvq.so xvq
    package require xvq
} 0.1

test 1 {xvq command} {
    xvq test ha
} ha

tcltest::cleanupTests
