#! /usr/bin/env tclkit
# timings for hashing and grouping

set f /tmp/hash.db
set N1 1000000
set N2 1000

# biggie
if 0 {
  # no hashing at all
  Mon Jun 26 20:01:30 CEST 2006 (/tmp/hash.db: 325711 b)
         2.0   group                                    (10x)
     92085.3   join v1 v2                               (1x)
  Mon Jun 26 20:03:02 CEST 2006 (times in msec, 10000 rows)
  # with hash value comparisons, but no hash table
  Mon Jun 26 20:34:45 CEST 2006 (/tmp/hash.db: 325711 b)
         3.8   group                                    (10x)
       879.1   join v1 v2                               (1x)
  Mon Jun 26 20:34:46 CEST 2006 (times in msec, 10000 rows)
  # with real hashing
  Tue Jun 27 00:39:11 CEST 2006 (/tmp/hash.db: 325711 b)
         4.4   group                                    (10x)
        90.6   join v1 v2                               (1x)
  Tue Jun 27 00:39:11 CEST 2006 (times in msec, 10000 rows)
  # with a million rows in v* and a thousand in w*
  Tue Jun 27 03:21:59 CEST 2006 (/tmp/hash.db: 44582432 b)
       308.0   group v1                                 (10x)
         0.5   group w1                                 (1000x)
      6657.8   join v1 v2 on ints                       (1x)
       527.8   join v1 w2 on ints                       (1x)
      5669.4   join w2 v1 on ints                       (1x)
      8085.7   join v1 v3 on strings                    (1x)
       943.8   join v1 w3 on strings                    (1x)
      5957.3   join w3 v1 on strings                    (1x)
  Tue Jun 27 03:22:31 CEST 2006 (times in msec, 1000000 rows)
  # v3
  Thu Sep 21 11:20:45 CEST 2006 (/tmp/hash.db: 42582542 b)
       734.9   group v1                                 (1x)
	 0.7   group w1                                 (1000x)
      1100.7   join v1 v2 on ints                       (1x)
       207.5   join v1 w2 on ints                       (1x)
       537.3   join w2 v1 on ints                       (1x)
      1760.5   join v1 v3 on strings                    (1x)
       324.1   join v1 w3 on strings                    (1x)
       788.1   join w3 v1 on strings                    (1x)
      5028.0   array fill                               (1x)
       998.1   array names                              (1x)
       506.7   array lookup 0x                          (1x)
      1304.1   array lookup 1x                          (1x)
      1739.4   array lookup 2x                          (1x)
      3824.8   unset array                              (1x)
  Thu Sep 21 11:21:05 CEST 2006 (times in msec, N1=1000000 N2=1000)
}

if {![file exists $f]} {
  # make the dataset

  puts "Creating $f"
  puts [clock format [clock seconds]]

  mk::file open db
  mk::view layout db.v1 {i:I s:S g:S}
  mk::view layout db.v2 {i:I t:S}
  mk::view layout db.v3 {s:S j:I}
  mk::view layout db.w1 {i:I s:S g:S}
  mk::view layout db.w2 {i:I t:S}
  mk::view layout db.w3 {s:S j:I}
  
  for {set i 0} {$i < $N1} {incr i} {
    mk::row append db.v1 i $i s s$i g g[string range $i 3 end] ;# 1111 groups
    mk::row append db.v2 i $i t t$i
    mk::row append db.v3 s s$i j ${i}0
    if {$i < $N2} {
      mk::row append db.w1 i $i s s$i g g[string range $i 1 end] ;# 111 groups
      mk::row append db.w2 i $i t t$i
      mk::row append db.w3 s s$i j ${i}0
    }
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
set v1 [view $db get 0 v1]
set v2 [view $db get 0 v2]
set v3 [view $db get 0 v3]
set w1 [view $db get 0 w1]
set w2 [view $db get 0 w2]
set w3 [view $db get 0 w3]

mk::file open db $f -readonly

if {$N1 != [mk::view size db.v1]} {
  puts "Data file row count was [mk::view size db.rows] (should be $N1)."
  puts "Please delete $f and restart this script."
  exit 1
}

proc t {msg cnt cmd} {
  set v1 $::v1
  set v2 $::v2
  set v3 $::v3
  set w1 $::w1
  set w2 $::w2
  set w3 $::w3
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

#puts [view $v1 group g x | size]
#puts [view $w1 group g x | size]

t {group v1} 1 {
  view $v1 group g x | size
}
t {group w1} 1000 {
  view $w1 group g x | size
}
t {join v1 v2 on ints} 1 {
  view $v1 join $v2 j | size
}
t {join v1 w2 on ints} 1 {
  view $v1 join $w2 j | size
}
t {join w2 v1 on ints} 1 {
  view $w2 join $v1 j | size
}
t {join v1 v3 on strings} 1 {
  view $v1 join $v3 j | size
}
t {join v1 w3 on strings} 1 {
  view $v1 join $w3 j | size
}
t {join w3 v1 on strings} 1 {
  view $w3 join $v1 j | size
}
t {array fill} 1 {
  global a
  array unset a
  array set a [view $v1 mapcols i g | get]
  #puts [array size a]
}
t {array names} 1 {
  global a an
  set an [array names a]
}
  proc p {} {
    global an
    foreach i $an {
      set j $i
    }
  }
t {array lookup 0x} 1 {
  p
}
  proc p {} {
    global a an
    foreach i $an {
      set j $a($i)
    }
  }
t {array lookup 1x} 1 {
  p
}
  proc p {} {
    global a an
    foreach i $an {
      set j $a($i)
      set j $a($i)
    }
  }
t {array lookup 2x} 1 {
  p
}
t {unset array} 1 {
  global a an
  unset a an
}

puts "[clock format [clock seconds]] (times in msec, N1=$N1 N2=$N2)"
