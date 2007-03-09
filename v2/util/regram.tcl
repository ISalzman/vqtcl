#!/usr/bin/env tclkit
# regram -=- take taccle output and convert it to table-driven parse data

set symbolic 0
if {[lindex $argv 0] eq "-s"} {
  set symbolic 1
  set argv [lrange $argv 1 end]
}

set fn [lindex $argv 0]
set pf [lindex $argv 1]

if {$fn eq ""} {
  puts stderr "Usage: $argv0 ?-s? infile ?prefix?"
  exit 1
}

source $fn

if {[info procs yyparse] ne ""} {
  set body [info body yyparse]
} else {
  upvar 0 zztable yytable
  upvar 0 zzrules yyrules
  set body [info body zzparse]
}

foreach {x y} [array get yyrules *,l] {
  set rul([regsub {,l} $x {}]) $y
}

foreach {x y} [array get yyrules *,dc] {
  set rdc([regsub {,dc} $x {}]) $y
}

if {[array size rul] > 256} { error "[array size rul] states, limit is 256" }

foreach {x y} [array get yytable] {
  regexp {^([^:]*):(.*)$} $x - a b
  switch $y {
    shift -
    reduce -
    goto -
    accept {
      if {![string is int $b]} { scan $b %c b }
      set y [lsearch {accept shift reduce goto} $y]
      set acts([expr {$b*256+$a}]) $y
    }
    default {
      if {[string match *,target $b]} {
	set b [regsub ,target $b {}]
	if {![string is int $b]} { scan $b %c b }
	set targ([expr {$b*256+$a}]) $y
      } else {
	puts "x $x y $y ?"
      }
    }
  }
}

if {[lsort [array names acts]] ne [lsort [array names targ]]} { error huh? }

foreach {x y} [array get targ] {
  set b [expr {$x/256}]
  set a [expr {$x%256}]
  set v $acts($x)
  set targ($x) [expr {$y*4+$v}]
  if {33 <= $b && $b < 127} { set b [format %c $b] }
  set acts($x) [list st-$a lx-$b tp-$v nx-$y]
}

foreach x [lsort -integer [array names rul]] { lappend lrul $rul($x) }
foreach x [lsort -integer [array names rdc]] { lappend lrdc $rdc($x) }

puts "  $fn: rul [array size rul] acts [array size acts] targ [array size targ]"

if {![regexp {switch -- \$[yz][yz]rule (.*) [yz][yz]unsetupvalues } $body - code]} { error uh? }

for {set i 0} {$i < [array size rul]} {incr i} { set codes($i) "  List  " }
array set codes [lindex $code 0]
set lnod {}
set ltab {}
foreach x [lsort -integer [array names codes]] {
  set c $codes($x)
  lappend lcod $c
  set typ [lindex $c 0]
  set c [lrange $c 1 end]
  switch $typ {
    Node {
      set typ [lindex $c 0]
      if {![info exists types($typ)]} {
	set types($typ) [llength $lnod]
	lappend lnod $typ
      }
      if {$types($typ) >= 256} { error "more than 256 node types?" }
      set v [expr {($types($typ)<<8)|([llength $c]<<4)}]
      foreach x [lrange $c 1 end] {
	set v [expr {$v|(1<<($x+16))}]
      }
    }
    List {
      set c [linsert $c 0 1 [llength $c]]
      set v 0
      set n -4
      foreach x $c { set v [expr {$v|($x<<[incr n 4])}] }
    }
    Join {
      set c [linsert $c 0 2 [llength $c]]
      set v 0
      set n -4
      foreach x $c { set v [expr {$v|($x<<[incr n 4])}] }
    }
    Pass {
      set c [linsert $c 0 3 [llength $c]]
      set v 0
      set n -4
      foreach x $c { set v [expr {$v|($x<<[incr n 4])}] }
    }
  }
  lappend ltab $v
}

set lkey {}
set lval {}
foreach {x y} [array get targ] {
  lappend lkey $x
  lappend lval $y
}

set fd [open [file root $fn].tab[file ext $fn]]
set lsym {}
while {[gets $fd line] >= 0} {
  if {[regexp {set ::(\w+) (\d+)} $line - sym val]} {
    lappend lsym $sym
  }
}
close $fd

set nalt [lsearch -exact $lsym _ALT_]
if {$nalt < 0} { error alt? }
set nops [lsearch -exact $lsym _OPS_]
if {$nops < 0} { error ops? }

set fd [open [file root $fn].dat[file ext $fn] w]

if {$symbolic} {
  puts $fd "set ${pf}lcod {"
  foreach x $lcod {
    puts $fd "  [list $x]"
  }
  puts $fd "}"
  puts $fd "array set ${pf}targ {"
  foreach {x y} [array get targ] {
    puts $fd "  [list $x $y]"
  }
  puts $fd "}"
  puts $fd ""
}

set lsym [lrange $lsym [expr {$nalt+1}] [expr {$nops-1}]]

puts $fd [list set ${pf}lrul $lrul]
puts $fd [list set ${pf}lrdc $lrdc]
puts $fd [list set ${pf}lnod $lnod]
puts $fd [list set ${pf}ltab $ltab]
puts $fd [list set ${pf}lsym $lsym]
puts $fd [list set ${pf}lkey $lkey]
puts $fd [list set ${pf}lval $lval]
close $fd

file delete [file root $fn].db
mk::file open db [file root $fn].db
mk::view layout db.states { r:I c:I t:I }
mk::view layout db.nodes { s:S }
mk::view layout db.words { s:S }
mk::view layout db.trans { k:I v:I }

foreach r $lrul c $lrdc t $ltab { mk::row append db.states r $r c $c t $t }
foreach s $lnod { mk::row append db.nodes s $s }
foreach s $lsym { mk::row append db.words s $s }
foreach k $lkey v $lval { mk::row append db.trans k $k v $v }

mk::file close db
