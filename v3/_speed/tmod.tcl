#! /usr/bin/env tclkit
# timings for modifications

set f /tmp/mod.db
set N 10000

# biggie
if 0 {
  Thu Sep 21 11:33:40 CEST 2006 (/tmp/mod.db: 73953 b)
	69.9   mk get i                                 (10x)
       133.7   mk get all                               (10x)
       134.6   mk get all named                         (10x)
       137.1   mk get all tagged                        (10x)
	34.2   vq get i trace                           (10x)
	63.1   vq get all trace                         (10x)
	88.1   vq get all vget                          (10x)
       101.4   vq get all vget tagged                   (10x)
	35.0   vq get dref trace i                      (10x)
	63.5   vq get dref trace                        (10x)
	91.8   vq get dref vget                         (10x)
       104.4   vq get dref vget tagged                  (10x)
       165.5   vq get modified trace i                  (10x)
       321.9   vq get modified trace                    (10x)
       359.5   vq get modified vget                     (10x)
       381.1   vq get modified vget tagged              (10x)
       264.0   vq get two mods trace i                  (10x)
       519.2   vq get two mods trace                    (10x)
       559.5   vq get two mods vget                     (10x)
       589.9   vq get two mods vget tagged              (10x)
  Thu Sep 21 11:34:22 CEST 2006 (times in msec, 10000 rows)
}

if {![file exists $f]} {
  # make the dataset

  puts "Creating $f"
  puts [clock format [clock seconds]]

  mk::file open db
  mk::view layout db.data {i:I s:S}
  
  for {set i 0} {$i < $N} {incr i} {
    mk::row append db.data i $i s $i
  }

  puts [clock format [clock seconds]]

  set fd [open $f w]
  mk::file save db $fd
  close $fd
  mk::file close db

  puts [clock format [clock seconds]]
  puts ""
}

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .
package require ratcl

puts "[clock format [clock seconds]] ($f: [file size $f] b)"

set db [view $f open]
set data [view $db get 0 data]
set dref {ref data}
  
mk::file open db $f -readonly

proc t {msg cnt cmd} {
  set data $::data
  set dref $::dref
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

t {mk get i} 10 {
  mk::loop c db.data { set x [mk::get $c i] }
}
t {mk get all} 10 {
  mk::loop c db.data { set x [mk::get $c i]; set y [mk::get $c s] }
}
t {mk get all named} 10 {
  mk::loop c db.data { foreach {x y} [mk::get $c i s] break }
}
t {mk get all tagged} 10 {
  mk::loop c db.data { foreach {- x - y} [mk::get $c] break }
}

t {vq get i trace} 10 {
  view $data loop { set x $(i) }
}
t {vq get all trace} 10 {
  view $data loop { set x $(i); set y $(s) }
}
t {vq get all vget} 10 {
  view $data loop { foreach {x y} [vget $data $(#) *] break }
}
t {vq get all vget tagged} 10 {
  view $data loop { foreach {- x - y} [vget $data $(#)] break }
}

t {vq get dref trace i} 10 {
  view $dref loop { set x $(i) }
}
t {vq get dref trace} 10 {
  view $dref loop { set x $(i); set y $(s) }
}
t {vq get dref vget} 10 {
  view $dref loop { foreach {x y} [vget $data $(#) *] break }
}
t {vq get dref vget tagged} 10 {
  view $dref loop { foreach {- x - y} [vget $data $(#)] break }
}

  view $dref set [expr {$N-1}] i -1 s ?
  
t {vq get modified trace i} 10 {
  view $::dref loop { set x $(i) }
}
t {vq get modified trace} 10 {
  view $::dref loop { set x $(i); set y $(s) }
}
t {vq get modified vget} 10 {
  view $dref loop { foreach {x y} [vget $data $(#) *] break }
}
t {vq get modified vget tagged} 10 {
  view $dref loop { foreach {- x - y} [vget $data $(#)] break }
}

  view $dref set 0 i -1 s ?
  
t {vq get two mods trace i} 10 {
  view $::dref loop { set x $(i) }
}
t {vq get two mods trace} 10 {
  view $::dref loop { set x $(i); set y $(s) }
}
t {vq get two mods vget} 10 {
  view $dref loop { foreach {x y} [vget $data $(#) *] break }
}
t {vq get two mods vget tagged} 10 {
  view $dref loop { foreach {- x - y} [vget $data $(#)] break }
}

puts "[clock format [clock seconds]] (times in msec, $N rows)"
