# common test setup script

if {[info exists testsInited]} return

package require Tcl 8.4

if {[lsearch [namespace children] ::tcltest] == -1} {
  package require tcltest 2.2
  namespace import tcltest::*
}

singleProcess true ;# run without forking

testsDirectory [file dirname [info script]]

# if run from the tests directory, move one level up
if {[pwd] eq [testsDirectory]} {
  cd ..
}

temporaryDirectory [pwd]
workingDirectory [file dirname [testsDirectory]]

set testsInited 1
