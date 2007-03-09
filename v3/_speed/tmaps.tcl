#! /usr/bin/env tclkit
# various ways to use maps

set N 1000000

# biggie
if 0 {
  Thu Sep 21 11:25:15 CEST 2006
	 0.0   create iota                              (1000x)
	 0.0   create view                              (10x)
      3275.5   iterate over view                        (1x)
      2543.1   convert to string                        (1x)
      3379.1   convert to list                          (1x)
      3330.9   create mixed up                          (1x)
      2864.8   sort list as ints                        (1x)
      2365.7   sort list as strings                     (1x)
      1388.9   sort view                                (1x)
      1974.1   sort mixed                               (1x)
  Thu Sep 21 11:25:37 CEST 2006 (times in msec, N = 1000000)
}

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .
package require ratcl

puts [clock format [clock seconds]]

proc t {msg cnt cmd} {
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

#puts [view $v1 group g x | size]
#puts [view $w1 group g x | size]
  
t {create iota} 1000 {
  set ::iota [vget $::N * #]
}
t {create view} 10 {
  set ::v [view N:I vdef $::iota]
}
t {iterate over view} 1 {
  view $::v loop { set a $(N) }
}
t {convert to string} 1 {
  string length [vget $::N * #]
}
t {convert to list} 1 {
  set ::l [vget $::N * #]
  llength $::l
}
t {create mixed up} 1 {
  set ::mix [view $::N collect { $(#) ^ 543210 } | asview N:I]
  vget $::mix #
}
t {sort list as ints} 1 {
  lsort -integer $::l
}
t {sort list as strings} 1 {
  lsort $::l
}
t {sort view} 1 {
  view $::v sort | size
}
t {sort mixed} 1 {
  view $::mix sort | size
}

puts "[clock format [clock seconds]] (times in msec, N = $N)"
