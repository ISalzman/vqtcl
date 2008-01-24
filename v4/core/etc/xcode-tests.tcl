# used by xcode with tclg from the build/{Debug,Release}/ directories

puts "Running [file normalize [info script]]"
lappend auto_path [pwd] /System/Library/Tcl

set argv {-v pe}
source ../../tests/all.tcl
#source ../../tests/ref.test

#puts [info loaded]
