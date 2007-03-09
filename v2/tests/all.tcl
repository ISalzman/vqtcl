#!/usr/bin/tclsh
# all.tcl -=- run all tests in this directory

set uses {}
source [file join [file dirname [info script]] initests.tcl]
runAllTests
