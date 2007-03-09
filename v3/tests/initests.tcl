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

# TextMate support on Mac OS X: run make before running any test from editor
if {[info exists env(TM_FILENAME)]} {
  exec make
}

proc locateSupportScripts {} {
  global auto_path
  set mainDir [file dirname [testsDirectory]]
  
  # make sure pkgIndex.tcl is in current dir so local shared lib gets loaded
  if {![file exists pkgIndex.tcl]} {
    file copy [file join $mainDir pkgIndex.tcl] .
  }
  # make sure this pkgIndex.tcl is found
  if {[lsearch $auto_path .] < 0} {
    lappend auto_path .
  }
  # also need to find the remaining scripts
  if {![file exists library]} {
    file link -sym library [file join $mainDir library]
  }
}

locateSupportScripts

testConstraint tcl$tcl_version  1
testConstraint metakit          [expr {![catch { package require Mk4tcl }]}]
testConstraint vfs              [expr {![catch { package require vfs }]}]

set testsInited 1
