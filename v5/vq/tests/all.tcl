#!/usr/bin/env tclkit

# run all tests

#puts [info patchlevel]
source [file join [file dir [info script]] initests.tcl]
runAllTests
