#!/usr/bin/env tclsh

source [file join [file dir [info script]] initests.tcl]

runAllTests

eval unset [info vars ?]
eval unset [info vars ??]
#puts [info vars *]
