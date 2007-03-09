#! /usr/bin/env tclkit
# timings for wikit dataset

set f /Users/Shared/Datasets/mk/wikit.tkd

# biggie
if 0 {
Thu Sep 21 11:28:30 CEST 2006 (/Users/Shared/Datasets/mk/wikit.tkd: 35656708 b)
     119.2   mk find date                             (10x)
      59.4   vq index date                            (10x)
      17.6   mk find date miss                        (10x)
       2.6   vq find date miss                        (100x)
     115.1   mk regex name                            (10x)
      83.1   vq regex name                            (10x)
      94.9   vq regex name nocase                     (10x)
      41.8   mk glob name                             (10x)
      72.3   vq glob name                             (10x)
      45.5   mk glob name nocase                      (10x)
      80.4   vq glob name nocase                      (10x)
     223.0   mk find name                             (10x)
      65.3   vq index name                            (10x)
      13.6   mk find name exact                       (10x)
       2.8   vq find name exact                       (100x)
    1509.1   mk regex pages                           (1x)
     831.7   vq regex pages                           (1x)
    1311.0   vq regex pages nocase                    (1x)
     672.3   mk glob pages                            (1x)
     755.6   vq glob pages                            (1x)
    1261.4   mk glob pages nocase                     (1x)
    1352.9   vq glob pages nocase                     (1x)
     384.0   mk find pages                            (1x)
     309.1   vq index pages                           (1x)
      14.5   mk find pages miss                       (10x)
       2.7   vq find pages miss                       (100x)
     566.2   mk find refs                             (1x)
     216.0   vq index refs                            (1x)
      41.4   mk find refs exact                       (10x)
       0.0   vq find refs exact                       (100x)
      41.5   mk find refs miss                        (10x)
       6.6   vq find refs miss                        (100x)
     839.5   mk sort name                             (1x)
     170.9   vq sort name                             (1x)
    2174.8   mk sort refs                             (1x)
     174.0   vq sort refs                             (1x)
Thu Sep 21 11:28:59 CEST 2006 (times in msec, 16050 pages, 64713 refs)
}

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .
package require ratcl

puts "[clock format [clock seconds]] ($f: [file size $f] b)"

set db [view $f open]
set pages [view $db get 0 pages]
set refs [view $db get 0 refs]

set NP [view $pages size]
set NR [view $refs size]

mk::file open db $f -readonly

proc t {msg cnt cmd} {
  set pages $::pages
  set refs $::refs
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

t {mk find date} 10 {
  mk::select db.pages date -1
}
t {vq index date} 10 {
  view $pages index { $(date) == -1 }
}
t {mk find date miss} 10 {
  mk::select db.pages -exact -count 1 date -1
}
t {vq find date miss} 100 {
  view $pages mapcols date | find -1
}
t {mk regex name} 10 {
  mk::select db.pages -regexp name wikit
}
t {vq regex name} 10 {
  view $pages index { [regexp wikit $(name)] }
}
t {vq regex name nocase} 10 {
  view $pages index { [regexp -nocase wikit $(name)] }
}
t {mk glob name} 10 {
  mk::select db.pages -glob name *wikit*
}
t {vq glob name} 10 {
  view $pages index { [string match *wikit* $(name)] }
}
t {mk glob name nocase} 10 {
  mk::select db.pages -globnc name *wikit*
}
t {vq glob name nocase} 10 {
  view $pages index { [string match -nocase *wikit* $(name)] }
}
t {mk find name} 10 {
  mk::select db.pages name wikit
}
t {vq index name} 10 {
  view $pages index { $(name) eq "wikit" }
}
t {mk find name exact} 10 {
  mk::select db.pages -exact -count 1 name wikit
}
t {vq find name exact} 100 {
  view $pages mapcols name | find wikit
}
t {mk regex pages} 1 {
  mk::select db.pages -regexp page wikit
}
t {vq regex pages} 1 {
  view $pages index { [regexp wikit $(page)] }
}
t {vq regex pages nocase} 1 {
  view $pages index { [regexp -nocase wikit $(page)] }
}
  # pre-scan to get data in memory
  mk::select db.pages -exact page ""
t {mk glob pages} 1 {
  mk::select db.pages -glob page *wikit*
}
t {vq glob pages} 1 {
  view $pages index { [string match *wikit* $(page)] }
}
t {mk glob pages nocase} 1 {
  mk::select db.pages -globnc page *wikit*
}
t {vq glob pages nocase} 1 {
  view $pages index { [string match -nocase *wikit* $(page)] }
}
t {mk find pages} 1 {
  mk::select db.pages page wikit
}
t {vq index pages} 1 {
  view $pages index { $(page) eq "wikit" }
}
t {mk find pages miss} 10 {
  mk::select db.pages -exact -count 1 page wikit
}
t {vq find pages miss} 100 {
  view $pages mapcols page | find wikit
}
  # pre-scan to get data in memory
  mk::select db.refs -exact from -1
  mk::select db.refs -exact to -1  
t {mk find refs} 1 {
  mk::select db.refs to 1620
}
t {vq index refs} 1 {
  view $refs index { $(to) == 1620 }
}
t {mk find refs exact} 10 {
  mk::select db.refs -exact -count 1 to 1620
}
t {vq find refs exact} 100 {
  view $refs mapcols to | find 1620
}
t {mk find refs miss} 10 {
  mk::select db.refs -exact -count 1 to -1
}
t {vq find refs miss} 100 {
  view $refs mapcols to | find -1
}
  mk::select db.pages -sort name
t {mk sort name} 1 {
  mk::select db.pages -sort name
}
t {vq sort name} 1 {
  view $pages sort name | size
  #puts [view $pages sort name | mapcols name date who | last 10000 | dump]
}
xt {mk sort page} 1 {
  mk::select db.pages -sort page
}
xt {vq sort page} 1 {
  view $pages sort page | size
}
  mk::select db.refs -sort {from to}
t {mk sort refs} 1 {
  mk::select db.refs -sort {from to}
}
t {vq sort refs} 1 {
  view $refs sort from to | size
  #puts [view $refs sort from to | dump]
}

#puts [llength [view $refs index { $(to) == 1620 }]]
#puts [llength [view $refs mapcols to | find 1620]]

puts "[clock format [clock seconds]] (times in msec, $NP pages, $NR refs)"
