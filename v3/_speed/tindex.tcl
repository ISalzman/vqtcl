#! /usr/bin/env tclkit
# timings for simple indexing

# biggie
if 0 {
  Sat Sep 23 12:50:10 CEST 2006
	 1.7   loop                                     (100x)
	 2.2   lindex 1                                 (100x)
	 2.7   lindex 2                                 (100x)
	 3.2   lindex 3                                 (100x)
	 3.0   vget 1                                   (100x)
	 4.3   vget 2                                   (100x)
	 5.6   vget 3                                   (100x)
	 5.6   mk 1                                     (100x)
	 9.2   mk 2                                     (100x)
	12.4   mk 3                                     (100x)
  Sat Sep 23 12:50:15 CEST 2006 (times in msec)
}

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .
package require ratcl

puts "[clock format [clock seconds]]"

set v [view A:I vdef {123456 234567 345678 456789} | freeze]
set l [list 123456 234567 345678 456789]

mk::file open db
mk::view layout db.v A:I
foreach x {123456 234567 345678 456789} {
  mk::row append db.v A $x
}

proc t {msg cnt cmd} {
  set v $::v
  set l $::l
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

t {loop} 100 {
  for {set i 0} {$i < 1000} {incr i} {
  }
}

t {lindex 1} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    lindex $l 1
  }
}
t {lindex 2} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    lindex $l 1
    lindex $l 2
  }
}
t {lindex 3} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    lindex $l 1
    lindex $l 2
    lindex $l 3
  }
}

t {vget 1} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    vget $v 1 A
  }
}
t {vget 2} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    vget $v 1 A
    vget $v 2 A
  }
}
t {vget 3} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    vget $v 1 A
    vget $v 2 A
    vget $v 3 A
  }
}

t {mk 1} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    mk::get db.v!1 A
  }
}
t {mk 2} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    mk::get db.v!1 A
    mk::get db.v!2 A
  }
}
t {mk 3} 100 {
  for {set i 0} {$i < 1000} {incr i} {
    mk::get db.v!1 A
    mk::get db.v!2 A
    mk::get db.v!3 A
  }
}

puts "[clock format [clock seconds]] (times in msec)"
