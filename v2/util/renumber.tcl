#!/usr/bin/env tclkit
# renumber -=- Utility script to renumber #define's, case's, etc.

# I.e. renumber.tcl adjusts ints found between "%renumber<regexp>%" and
# "%renumber<>%" markers to be used from vi as ":%!renumber.tcl"

set match ""
while {[gets stdin line] >= 0} {
  if {[regexp {%renumber<(.*)>%} $line - match]} {
    set rc -1
  } elseif {$match ne "" && [regexp $match $line]} {
    regsub {\d+} $line [incr rc] line
  }
  puts $line
}
