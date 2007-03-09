#! /usr/bin/env tclkit
# timings for the one-million-row dataset

set f /tmp/omr.db
#set N 1000000
set N 100000

if {![file exists $f]} {
  # make the one-million-row dataset

  puts "Creating $f"
  puts [clock format [clock seconds]]

  mk::file open db
  mk::view layout db.rows {i0:I i1:I i2:I i4:I i8:I i16:I i32:I \
                            l:L u:I r:I f:F d:D e:S c:S o:S n:B b:B x:B}
  mk::view layout db.onei {r:I}
  mk::view layout db.ones {r:S}
  mk::view layout db.oneb {r:B}
  mk::view layout db.onez {i0:I}

  set r 1
  for {set i 0} {$i < $N} {incr i} {
    set r [expr {int(211*$r)}]
    mk::row append db.rows i0 0 i1 1 i2 3 i4 15 i8 127 i16 32767 i32 1000000 \
                            l 1 u $i r $r f 1 d 1 e "" c C o 1 n "" b B x 1
    mk::row append db.onei r $r
    mk::row append db.ones r $r
    mk::row append db.oneb r $r
  }
  mk::view size db.onez $N

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
  Thu Sep 21 11:22:37 CEST 2006 (/tmp/omr.db: 6858373 b)
	 8.4   vq find i0 miss                          (10x)
       106.6   mk find i0 miss                          (10x)
	12.2   vq find onei miss                        (10x)
       107.0   mk find onei miss                        (10x)
	18.3   vq find ones miss                        (10x)
       106.4   mk find ones miss                        (10x)
       296.2   vq loop -where i0 match                  (1x)
       303.7   vq loop -index i0 match                  (1x)
       295.8   mk select -exact i0 match                (1x)
       630.3   mk select i0 match                       (1x)
       309.8   vq loop -index i0 miss nested            (1x)
       310.3   vq loop -index i0 miss 2x                (1x)
       134.3   mk select -exact i0 miss 2x              (1x)
       733.0   mk select i0 miss 2x                     (1x)
       311.8   vq loop -where i0 miss                   (1x)
       300.6   vq loop -index i0 miss                   (1x)
       143.0   mk select -exact i0 miss                 (1x)
       726.6   mk select i0 miss                        (1x)
	 0.4   vq sum_8 col4                            (1000x)
	 0.6   vq sum_8w col4                           (1000x)
	 3.6   vq sum_v col4                            (1000x)
	38.1   tcl var sum for proc                     (1x)
	85.4   tcl array sum for proc                   (1x)
       104.0   tcl list sum for proc                    (1x)
       293.6   vq i0 sum loop trace proc                (1x)
       627.7   mk i0 sum proc                           (1x)
       487.4   tcl sum foreach                          (1x)
       480.8   vq i0 sum for vget                       (1x)
       518.1   vq i0 sum loop vget                      (1x)
       329.5   vq i0 sum loop trace                     (1x)
       339.9   vq i8 sum loop trace                     (1x)
       344.9   vq i32 sum loop trace                    (1x)
       686.3   mk i0 sum loop                           (1x)
       688.2   mk i8 sum loop                           (1x)
       701.2   mk i32 sum loop                          (1x)
       608.4   vq x:B sum loop trace                    (1x)
      1016.5   mk x:B sum loop                          (1x)
       538.4   vq o:S sum loop trace                    (1x)
       939.9   mk o:S sum loop                          (1x)
	16.9   tcl setup onez                           (1x)
	65.5   tcl setup onez expand                    (1x)
       184.3   tcl sort onez as int                     (1x)
       182.3   tcl sort onez as string                  (1x)
	30.9   tcl setup onei                           (1x)
       275.7   tcl setup onei expand                    (1x)
       425.9   tcl sort onei as int                     (1x)
       569.5   tcl sort onei as string                  (1x)
	62.6   tcl setup ones                           (1x)
       272.5   tcl setup ones expand                    (1x)
       617.9   tcl sort ones as int                     (1x)
       557.7   tcl sort ones as string                  (1x)
       130.0   tcl setup oneb                           (1x)
       389.5   tcl setup oneb expand                    (1x)
       692.3   tcl sort oneb as int                     (1x)
       625.3   tcl sort oneb as string                  (1x)
       215.2   vq view sort onei                        (1x)
      3770.7   mk select -sort onei                     (1x)
       966.9   vq view sort ones                        (1x)
      6816.6   mk select -sort ones                     (1x)
       526.8   vq view sort oneb                        (1x)
      4082.7   mk select -sort oneb                     (1x)
       148.2   vq view sort i0                          (1x)
       134.8   vq view sort onez                        (1x)
      1779.6   mk select -sort onez                     (1x)
       248.8   vq view sort r                           (1x)
      3387.6   mk select -sort r                        (1x)
  Thu Sep 21 11:23:27 CEST 2006 (times in msec, N = 100000)
}
# teevie
if 0 {
  Wed Jun 07 01:28:49 CEST 2006 (/tmp/omr.db: 42625264 b)
      864004 uS   vq loop -where i0 match                  (1x)
      886039 uS   vq loop -index i0 match                  (1x)
      879352 uS   mk select -exact i0 match                (1x)
     1271484 uS   mk select i0 match                       (1x)
      837499 uS   vq loop -index i0 miss nested            (1x)
      924811 uS   vq loop -index i0 miss 2x                (1x)
      217625 uS   mk select -exact i0 miss 2x              (1x)
     1643677 uS   mk select i0 miss 2x                     (1x)
      858116 uS   vq loop -where i0 miss                   (1x)
      855030 uS   vq loop -index i0 miss                   (1x)
      268243 uS   mk select -exact i0 miss                 (1x)
     1679164 uS   mk select i0 miss                        (1x)
        1245 uS   vq sum_8 col4                            (1x)
        1338 uS   vq sum_8w col4                           (1x)
        4041 uS   vq sum_i col0                            (1x)
        4662 uS   vq sum_i col4                            (1x)
       24586 uS   vq sum_o col0                            (1x)
       25217 uS   vq sum_o col4                            (1x)
       39632 uS   vq sum_v col0                            (1x)
       40716 uS   vq sum_v col4                            (1x)
      194052 uS   tcl var sum for proc                     (1x)
      262688 uS   tcl array sum for proc                   (1x)
      288328 uS   tcl list sum for proc                    (1x)
      802692 uS   vq i0 sum loop trace proc                (1x)
     1753418 uS   mk i0 sum proc                           (1x)
      994547 uS   tcl sum foreach                          (1x)
      841200 uS   tcl var sum for                          (1x)
      902272 uS   tcl array sum for                        (1x)
      938856 uS   tcl list sum for                         (1x)
     1580760 uS   vq i0 sum for vget                       (1x)
     1623753 uS   vq i0 sum loop vget                      (1x)
      932118 uS   vq i0 sum loop trace                     (1x)
      932895 uS   vq i8 sum loop trace                     (1x)
     1904960 uS   mk i0 sum loop                           (1x)
     1934380 uS   mk i8 sum loop                           (1x)
     1412776 uS   vq x:B sum loop trace                    (1x)
     2235537 uS   mk x:B sum loop                          (1x)
     1483401 uS   vq o:S sum loop trace                    (1x)
     2164956 uS   mk o:S sum loop                          (1x)
     4212565 uS   vq view sort i0                          (1x)
    78647539 uS   vq view sort r                           (1x)
    12333514 uS   mk select -sort r                        (1x)
  Wed Jun 07 01:30:59 CEST 2006
}

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .
package require ratcl

puts "[clock format [clock seconds]] ($f: [file size $f] b)"

set db [view $f mapf]
set rows [vget $db 0 rows]
set onei [vget $db 0 onei]
set ones [vget $db 0 ones]
set oneb [vget $db 0 oneb]
set onez [vget $db 0 onez]

mk::file open db $f -readonly

if {$N != [mk::view size db.rows]} {
  puts "Data file row count was [mk::view size db.rows] (should be $N)."
  puts "Please delete $f and restart this script."
  exit 1
}

proc t {msg cnt cmd} {
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

t {vq find i0 miss} 10 {
  view $::rows mapcols i0 | find -1
}
t {mk find i0 miss} 10 {
  mk::select db.rows -exact -count 1 i0 -1
}
t {vq find onei miss} 10 {
  view $::onei find -1
}
t {mk find onei miss} 10 {
  mk::select db.rows -exact -count 1 onei -1
}
t {vq find ones miss} 10 {
  view $::ones find -1
}
t {mk find ones miss} 10 {
  mk::select db.rows -exact -count 1 ones -1
}

t {vq loop -where i0 match} 1 {
  view $::rows where {$(i0) == 0}
}
t {vq loop -index i0 match} 1 {
  view $::rows index {$(i0) == 0}
}
t {mk select -exact i0 match} 1 {
  mk::select db.rows -exact i0 0
}
t {mk select i0 match} 1 {
  mk::select db.rows i0 0
}
t {vq loop -index i0 miss nested} 1 {
  view $::rows where {$(i0) == -1} | index {$(i0) == -1}
}
t {vq loop -index i0 miss 2x} 1 {
  view $::rows index {$(i0) == -1 && $(i0) == -1}
}
t {mk select -exact i0 miss 2x} 1 {
  mk::select db.rows -exact i0 -1 -exact i0 -1
}
t {mk select i0 miss 2x} 1 {
  mk::select db.rows i0 -1 i0 -1
}
t {vq loop -where i0 miss} 1 {
  view $::rows where {$(i0) == -1}
}
t {vq loop -index i0 miss} 1 {
  view $::rows index {$(i0) == -1}
}
t {mk select -exact i0 miss} 1 {
  mk::select db.rows -exact i0 -1
}
t {mk select i0 miss} 1 {
  mk::select db.rows i0 -1
}

vlerq::sum_8 $::rows 4

t {vq sum_8 col4} 1000 {
  vlerq::sum_8 $::rows 4
}
t {vq sum_8w col4} 1000 {
  vlerq::sum_8w $::rows 4
}
xt {vq sum_v col0} 1000 {
  vlerq::sum_v $::rows 0
}
t {vq sum_v col4} 1000 {
  vlerq::sum_v $::rows 4
}

  proc p {n} {
    set v 1
    set i 0
    for {set k 0} {$k < $n} {incr k} { incr i $v }
    return $i
  }
t {tcl var sum for proc} 1 {
  p [vget $::rows #]
}
  proc p {n} {
    array set v {x 1}
    set i 0
    for {set k 0} {$k < $n} {incr k} { incr i $v(x) }
    return $i
  }
t {tcl array sum for proc} 1 {
  p [vget $::rows #]
}
  proc p {n} {
    set v [list 1]
    set i 0
    for {set k 0} {$k < $n} {incr k} { incr i [lindex $v 0] }
    return $i
  }
t {tcl list sum for proc} 1 {
  p [vget $::rows #]
}

  proc p {rows} {
    set i 0
    view $rows loop { incr i $(i0) }
    return $i
  }
t {vq i0 sum loop trace proc} 1 {
  p $::rows
}
  proc p {} {
    set i 0
    mk::loop c db.rows { incr i [mk::get $c i0] }
    return $i
  }
t {mk i0 sum proc} 1 {
  p
}

  set o [split [string repeat 1 [vget $rows #]] ""]
t {tcl sum foreach} 1 {
  set i 0
  foreach v $::o { incr i $v }
}
  unset o

xt {tcl var sum for} 1 {
  set v 1
  set i 0
  set n [vget $::rows #]
  for {set k 0} {$k < $n} {incr k} { incr i $v }
}
xt {tcl array sum for} 1 {
  array set v {x 1}
  set i 0
  set n [vget $::rows #]
  for {set k 0} {$k < $n} {incr k} { incr i $v(x) }
}
xt {tcl list sum for} 1 {
  set v [list 1]
  set i 0
  set n [vget $::rows #]
  for {set k 0} {$k < $n} {incr k} { incr i [lindex $v 0] }
}

xt {tcl for loop scalar} 1 {
  set j 0
  for {set i 0} {$i < $::N} {incr i} { incr j $i }
}
xt {tcl for loop array} 1 {
  set j 0
  for {set a(1) 0} {$a(1) < $::N} {incr a(1)} { incr j $a(1) }
}
xt {vq counted loop} 1 {
  set j 0
  view $::N loop { incr j $(#) }
}
xt {tcl for loop scalar 3x} 1 {
  set j 0
  for {set i 0} {$i < $::N} {incr i} { incr j $i; incr j $i; incr j $i }
}
xt {tcl for loop array 3x} 1 {
  set j 0
  for {set a(1) 0} {$a(1) < $::N} {incr a(1)} { 
    incr j $a(1); incr j $a(1); incr j $a(1)
  }
}
xt {vq counted loop 3x} 1 {
  set j 0
  view $::N loop { incr j $(#); incr j $(#); incr j $(#) }
}

t {vq i0 sum for vget} 1 {
  set i 0
  set rows $::rows
  set n [vget $rows #]
  for {set k 0} {$k < $n} {incr k} { incr i [vget $rows $k i0] }
}
t {vq i0 sum loop vget} 1 {
  set i 0
  set rows $::rows
  view $rows loop { incr i [vget $rows $(#) i0] }
}

t {vq i0 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i0) }
}
xt {vq i1 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i1) }
}
xt {vq i2 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i2) }
}
xt {vq i4 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i4) }
}
t {vq i8 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i8) }
}
xt {vq i16 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i16) }
}
t {vq i32 sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(i32) }
}
  
t {mk i0 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i0] }
}
xt {mk i1 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i1] }
}
xt {mk i2 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i2] }
}
xt {mk i4 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i4] }
}
t {mk i8 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i8] }
}
xt {mk i16 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i16] }
}
t {mk i32 sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c i32] }
}

t {vq x:B sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(x) }
}
t {mk x:B sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c x] }
}
t {vq o:S sum loop trace} 1 {
  set i 0
  view $::rows loop { incr i $(o) }
}
t {mk o:S sum loop} 1 {
  set i 0
  mk::loop c db.rows { incr i [mk::get $c o] }
}

t {tcl setup onez} 1 {
  set o [vget $::onez * 0]
}
t {tcl setup onez expand} 1 {
  string length [vget $::onez * 0]
}
t {tcl sort onez as int} 1 {
  set o [vget $::onez * 0]
  lsort -integer $o
}
t {tcl sort onez as string} 1 {
  set o [vget $::onez * 0]
  lsort $o
}
t {tcl setup onei} 1 {
  set o [vget $::onei * 0]
}
t {tcl setup onei expand} 1 {
  string length [vget $::onei * 0]
}
t {tcl sort onei as int} 1 {
  set o [vget $::onei * 0]
  lsort -integer $o
}
t {tcl sort onei as string} 1 {
  set o [vget $::onei * 0]
  lsort $o
}
t {tcl setup ones} 1 {
  set o [vget $::ones * 0]
}
t {tcl setup ones expand} 1 {
  string length [vget $::ones * 0]
}
t {tcl sort ones as int} 1 {
  set o [vget $::ones * 0]
  lsort -integer $o
}
t {tcl sort ones as string} 1 {
  set o [vget $::ones * 0]
  lsort $o
}
t {tcl setup oneb} 1 {
  set o [vget $::oneb * 0]
}
t {tcl setup oneb expand} 1 {
  string length [vget $::oneb * 0]
}
t {tcl sort oneb as int} 1 {
  set o [vget $::oneb * 0]
  lsort -integer $o
}
t {tcl sort oneb as string} 1 {
  set o [vget $::oneb * 0]
  lsort $o
}

t {vq view sort onei} 1 {
  view $::onei sortmap
}
t {mk select -sort onei} 1 {
  mk::select db.onei -sort r
}
t {vq view sort ones} 1 {
  view $::ones sortmap
}
t {mk select -sort ones} 1 {
  mk::select db.ones -sort r
}
t {vq view sort oneb} 1 {
  view $::oneb sortmap
}
t {mk select -sort oneb} 1 {
  mk::select db.oneb -sort r
}

t {vq view sort i0} 1 {
  view $::rows mapcols i0 | sortmap
}
t {vq view sort onez} 1 {
  view $::onez mapcols i0 | sortmap
}
t {mk select -sort onez} 1 {
  mk::select db.onez -sort i0
}
t {vq view sort r} 1 {
  view $::rows mapcols r | sortmap
}
t {mk select -sort r} 1 {
  mk::select db.rows -sort r
}

puts "[clock format [clock seconds]] (times in msec, N = $N)"
