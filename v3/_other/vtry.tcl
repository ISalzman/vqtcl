#! /usr/bin/env tclkit

#load timing/libvlerq1.dylib
lappend auto_path .
package require vlerq

set vops(blah) {::blah ::blah-s ::blah-g}
proc ::blah {x} {
  list blah {A B} blah
}
proc ::blah-s {data} {
  return 1000
}
proc ::blah-g {data row col} {
  return x
}

proc t1 {} {
  set v [view - blah]
puts A
  puts v-$v
puts B

  puts x-[view $v get #]
puts C
  puts y-[view $v get -1 B]
  puts z-[llength [view $v get * 0]]

  puts X-[time { view $v get # } 100000]
  puts Y-[time { view $v get 0 0 } 100000]
  puts Z-[time { llength [view $v get * 0] } 100]

  puts sd-[time { blah_sizer x } 100000]
  puts gd-[time { blah_getter x 0 0 } 100000]
}
t1

proc vlerq::ex1 {size args} {
  list ex1 $args $size
}
proc vlerq::sizer::ex1 {data} {
  return [lindex $data 2]
}
proc vlerq::getter::ex1 {data row col} {
  return "This is row $row, column [lindex $data 1 $col]"
}

set v {vop ex1 7 A B C}
puts [view $v get #]
puts [view $v get 3 B]
puts [view $v get @ * 0]

proc vlerq::sort1 {vin args} {
  set meta [view $vin get @]
  set sortmap {1 2 4 0 3} ;#[lsort -indices [view $vin get * 0]]
  list sort1 $meta $vin $sortmap
}
proc vlerq::sizer::sort1 {data} {
  set vin [lindex $data 2]
  return [view $vin get #]
}
proc vlerq::getter::sort1 {data row col} {
  set vin [lindex $data 2]
  set sortmap [lindex $data 3]
  return [view $vin get [lindex $sortmap $row] $col]
}
puts ===============

set v {vdef {A B} 3 three
                  0 zero
                  1 one
                  4 four
                  2 two   }
set v {vdef {A B} {3 0 1 4 2} {three zero one four two}}
puts [view $v get #]
puts [view $v get * A]
puts [view $v get * B]

set w [list vop sort1 $v]
# vop sort1 {vdef {A B} {3 0 1 4 2} {three zero one four two}}
  
#oputs [::vop::sort1 $v]

puts [view $w get #]
puts [view $w get * A]
puts [view $w get * B]



proc vlerq::fstat {path {pattern *}} {
  set meta {name size mtime ino mode type uid}
  set files [glob -tails -dir $path $pattern]
  list fstat $meta $path $files
}
proc vlerq::sizer::fstat {data} {
  return [llength [lindex $data 3]]
}
proc vlerq::getter::fstat {data row col} {
  set sb(name) [lindex $data 3 $row]
  file stat [lindex $data 2]/$sb(name) sb
  set colname [lindex $data 1 $col]
  #puts "$colname #$col: [array get sb]"
  return $sb($colname)
}

set v {vop fstat /usr/bin}
puts fstat-[view $v get #]
puts [view $v get 0]
puts [view $v get @ #]
puts [view $v get @ *]

puts ====================
set i 0
view /usr/bin fstat | loop c {
  puts [format {%10d  %s} $c(size) $c(name)]
  if {[incr i] == 10} break
}

puts ====================
set i 0
view /usr/bin fstat | sort1 | loop c {
  puts [format {%10d  %s} $c(size) $c(name)]
  if {[incr i] == 5} break
}
