#!/usr/bin/env tclkit

# run all tests

#puts [info patchlevel]
source [file join [file dir [info script]] initests.tcl]
runAllTests

# get rid of all single-character variables to release any associated memory
eval unset [info vars ?]