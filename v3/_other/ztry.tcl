#! /usr/bin/env tclkit

#load ../timing/libvlerq1.dylib
lappend auto_path ..
package require vlerq

set v [view {A B} vdef {0 zero 1 one 2 two 3 three 4 four}]
set v [view ../tests/l.kit-be mapf]
set v [view kitten mapf]

set dirs [view $v get 0 dirs]
puts "dirs $dirs"

proc t1 {} {
  set N 1000
  set dirs $::dirs
  set n [view $dirs get #]
  set v {}
  for {set i 0} {$i < $n} {incr i} { lappend v $i }
  puts tN-$n
  puts tb-[time { foreach x $v { } } $N]
  puts t0-[time { foreach x $v { set a 1 } } $N]
  puts t1-[time { foreach x $v { set a 1; set a 2 } } $N]
  puts t2-[time { view $dirs loop c { } } $N]
  puts t3-[time { view $dirs loop c { set a 1 } } $N]
  puts t4-[time { view $dirs loop c { set a 1; set a 2 } } $N]
  puts t5-[time { view $dirs loop c { set a $c(#) } } $N]
  puts t6-[time { view $dirs loop c { set a $c(parent) } } $N]
  puts t7-[time { view $dirs loop c \
                                { set a [view $dirs get $c(#) parent] } } $N]
  puts t8-[time { view $dirs loop c { set a [view $dirs get 1 parent] } } $N]
  puts t9-[time {
    set i 0; view $dirs loop c { set a [view $dirs get $i parent]; incr i }
	} $N]
  puts a1-[time { 
    set r {}; view $dirs loop c { lappend r $c(parent) } 
  } $N]
}
t1
if 0 {#; biggie
  tN-240
  tb-313.007 microseconds per iteration
  t0-475.874 microseconds per iteration
  t1-655.22 microseconds per iteration
  t2-145.373 microseconds per iteration
  t3-286.049 microseconds per iteration
  t4-427.871 microseconds per iteration
  t5-774.167 microseconds per iteration
  t6-878.174 microseconds per iteration
  t7-1253.846 microseconds per iteration
  t8-744.135 microseconds per iteration
  t9-872.364 microseconds per iteration
  a1-1074.034 microseconds per iteration
}
if 0 {;# teevie
  tN-240
  tb-158.379 microseconds per iteration
  t0-230.328 microseconds per iteration
  t1-335.379 microseconds per iteration
  t2-77.398 microseconds per iteration
  t3-154.733 microseconds per iteration
  t4-247.409 microseconds per iteration
  t5-430.331 microseconds per iteration
  t6-471.643 microseconds per iteration
  t7-419.108 microseconds per iteration
  t8-208.132 microseconds per iteration
  t9-250.725 microseconds per iteration
  a1-313.084 microseconds per iteration
}

#set i 0
#view $dirs loop c { puts A; if {[incr i] == 3} break; puts B }; puts C

#set i 0
#view $dirs loop c { puts a; if {[incr i] == 3} continue; puts b }; puts c

proc t2 {} {
  set v [view tests/l.kit-be mapf]
  set dirs [view $v get 0 dirs]
  view $dirs loop c { puts x-$c(parent) }
  view $dirs loop c { puts y-$c(#) }
  parray c
  puts [array names c]
  #puts [array statistics c]
  puts [info exists c(parent)]
  puts [info exists c(name)]
  view $dirs loop c { puts X-$c(parent) } { puts Y-$c(#) }
  view $dirs loop c { puts "$c(#): $c(parent) $c(name) [view $c(files) get #]" }
}
t2
