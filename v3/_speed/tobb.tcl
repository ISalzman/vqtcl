#! /usr/bin/env tclkit
# timings for the one-billion-byte dataset

set f /tmp/obb.db
#set N 250000000
set N 2500000

if {![file exists $f]} {
  # make the one-million-row dataset

  puts "Creating $f"
  puts [clock format [clock seconds]]

  mk::file open db
  mk::view layout db.data i:I
  mk::view size db.data $N
  mk::set db.data!0 i 123456789

  puts [clock format [clock seconds]]

  set fd [open $f w]
  mk::file save db $fd
  close $fd
  mk::file close db

  puts [clock format [clock seconds]]
  puts ""
}

# biggie
if 0 {
Thu Sep 21 11:30:57 CEST 2006 (/tmp/obb.db: 10000051 b)
     288.3   vq find miss                             (1x)
     283.4   vq find on cremap 2 cols miss            (1x)
     272.3   vq find on cremap 3 cols miss            (1x)
    6893.1   vq index miss                            (1x)
    1545.2   mk oo find miss                          (1x)
    2942.4   mk select miss                           (1x)
     808.4   vq project                               (1x)
     791.2   vq unique                                (1x)
    1613.6   vq unique on cremap 2 cols               (1x)
    1617.6   vq unique on cremap 3 cols               (1x)
Thu Sep 21 11:31:14 CEST 2006 (times in msec, N = 2500000)
}

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .
package require ratcl

puts "[clock format [clock seconds]] ($f: [file size $f] b)"

set db [view $f mapf]
set data [vget $db 0 data]

mk::file open db $f -readonly
set mkdata [mk::view open db.data]

if {$N != [mk::view size db.data]} {
  puts "Data file row count was [mk::view size db.data] (should be $N)."
  puts "Please delete $f and restart this script."
  exit 1
}

proc t {msg cnt cmd} {
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

t {vq find miss} 1 {
  view $::data find -1
}
t {vq find on cremap 2 cols miss} 1 {
  view $::data mapcols i i | find -1 -1
}
t {vq find on cremap 3 cols miss} 1 {
  view $::data mapcols i i i | find -1 -1 -1
}
xt {vq find modified view miss} 1 {
  view $::data set 0 i 987654321 | find -1
}
t {vq index miss} 1 {
  view $::data index { $(i) == -1 }
}  
t {mk oo find miss} 1 {
  catch { $::mkdata find i -1 }
}
t {mk select miss} 1 {
  mk::select db.data -exact i -1
}
t {vq project} 1 {
  view $::data project i | get #
}
t {vq unique} 1 {
  view $::data unique | get #
}
t {vq unique on cremap 2 cols} 1 {
  view $::data unique i i | get #
}
t {vq unique on cremap 3 cols} 1 {
  view $::data unique i i i | get #
}

puts "[clock format [clock seconds]] (times in msec, N = $N)"
