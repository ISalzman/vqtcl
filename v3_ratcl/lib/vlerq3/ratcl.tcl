# ratcl.tcl -- Relational algebra ond other utility operators for vlerq

package provide ratcl 3
package require vlerq

namespace eval ratcl {
  namespace export vopdef

  # define a view operator
  proc vopdef {name a1 {a2 "?"} {a3 "?"}} {
    global vops
    set context [uplevel { namespace current }]
    if {$context ne "::"} { append context :: }
    if {$a2 eq "?"} {
      foreach {t a b} $a1 {
        proc $context$name-$t $a $b
        set o($t) $context$name-$t
      }
      if {[info exists o(init)]} {
        set vops($name) [list $o(init) $o(sizer) $o(getter)]
      }
    } else {
      set vops($name) [list $context$name-vop]
      if {$a3 ne "?"} {
        lappend vops($name) $a1
        proc $context$name-vop $a2 $a3
      } else {
        proc $context$name-vop $a1 $a2
      }
    }
  }
  # return a list of unique indices
  proc vuniqmap {v av} {
    set w $v
    if {[llength $av] > 0} {
      set w [optcrm $v $av]
    }
    lindex [vlerq::vgroup "" $w $w] 0
  }
  # return a string describing the meta view structure
  proc vstruct {m} {
    set o ""
    view $m loop c {
      set t $c(type)
      if {$t eq "V" && [vget $c(subv) #] > 0} {
        set t ([vstruct $c(subv)])
      }
      append o $t
    }
    return $o
  }
  # check that two views are cmpatible enough
  proc vcompat {v w} {
    if {[vstruct [vget $v @]] ne [vstruct [vget $w @]]} {
      error "incompatible columns"
    }
  }
  # core view sort routine, returns a map vector
  proc vsort {v a} {
    if {[llength $a] > 0} {
      set v [optcrm $v $a]
    }
    view $v sortmap ;# TODO: no descending sorts yet, nor -nc
  }
  # core join code, relies on vgroup for the heavy lifting
  proc vjoin {v vc w wc jcol} {
    set vf [optcrm $v $vc]
    set wf [optcrm $w $wc]
    set wo [optcrm $w [linsert $wc 0 -omit]]
    vcompat $vf $wf
    foreach {gm em fm dm mm} [vlerq::vgroup df $wf $vf $wo] break
    # FIXME: same code also in the group operator
    set gg [view {} vdata [list $jcol V [list [vget $wo @]]]]
    view $v pair [view $gg vdata [list $mm] | remap $dm]
  }
  # return the list of column names common to both views
  proc vcommon {v w} {
    view $v get @ | cremap 0 | intersect [view $w get @ | cremap 0] | get
  }
  # a version of cremap which optimizes dummy remaps away
  proc optcrm {v a} {
    view $v cremap [view $v colnum $a] ;# TODO: move to cremap
  }
  # construct a MK-compatible description of a meta view
  proc vdesc {m} {
    set o ""
    set s ""
    view $m loop c {
      append o $s $c(name)
      set s ,
      set t $c(type)
      if {$t eq "V"} {
        if {[vget $c(subv) #] > 0} {
          set t "\[[vdesc $c(subv)]\]"
        } else {
          set t {[]}
        }
      } else {
        append o :
      }
      append o $t
    }
    return $o
  }
  # convert a view structure to Metakit format
  namespace eval save {
    proc asmk {v} {
      namespace eval v {}
      switch $::tcl_platform(byteOrder) {
        bigEndian    { set v::dat LJ }
        littleEndian { set v::dat JL }
      }
      append v::dat [binary format ccI 0x1a 0x0 0]
      set v::pos 8 ;# fill in header later
      set v::buf {}
      set p [edone [esub $v 1]]
      set s [expr {$v::pos - $p}]
      set o [string replace $v::dat 4 7 [binary format I [expr {$v::pos + 16}]]]
      append o [binary format IIII 0x80000000 $v::pos [expr {$s+0x80000000}] $p]
      namespace forget v
      return $o
    }
    proc ecol {v k} {
      switch [vget $v @ $k 1] {
        I { fixcol $v $k 1 }
        L { fixcol $v $k 2 }
        F { fixcol $v $k 3 }
        D { fixcol $v $k 4 }
        S { varcol $v $k 1 }
        B { varcol $v $k 0 }
        V { subcol $v $k }
        default { error "col $k, type [vget $v @ $c 1]?"}
      }
    }
    proc fixcol {v k t} {
      set opos $v::pos
      emit [vlerq::ibytes [vget $v * $k] $t]
      pkeep $opos
    }
    proc varcol {v k t} {
      set d [vget $v * $k]
      set o {}
      set opos $v::pos
      foreach x $d {
        set n [string length $x]
        if {$n > 0} {
          emit $x
          if {$t} { em1b 0; incr n }
        }
        lappend o $n
      }
      if {[pkeep $opos] > 0} {
        set opos $v::pos
        emit [vlerq::ibytes $o 1]
        pkeep $opos
      }
      pkeep $v::pos ;# no memos
    }
    proc subcol {v k} {
      set o {}
      view $v loop c { lappend o [esub [vget $v $c(#) $k] 0] }
      set opos $v::pos
      foreach x $o { edone $x }
      pkeep $opos
    }
    proc esub {v f} {
      set obuf $v::buf
      set v::buf {}
      set n [vget $v #]
      if {$n > 0} { view $v get @ | loop c { ecol $v $c(#) } }
      if {$f} { set d [ratcl::vdesc [vget $v @]] } else { set d "" }
      set b $v::buf
      set v::buf $obuf
      list $d $n $b
    }
    proc edone {e} {
      foreach {d n b} $e break
      set opos $v::pos
      em1v 0
      if {$d ne ""} {
        em1v [string length $d]
        emit $d
      }
      em1v $n
      if {$n > 0} { foreach x $b { em1v $x } }
      return $opos
    }
    proc pkeep {o} {
      set n [expr {$v::pos - $o}]
      lappend v::buf $n
      if {$n > 0} { lappend v::buf $o }
      return $n
    }
    proc emit {s} {
      append v::dat $s
      incr v::pos [string length $s]
    }
    proc em1b {c} {
      append v::dat [binary format c $c]
      incr v::pos
    }
    proc em1v {n} {
      if {$n < 0} { em1b 0; set n [expr {~$n}]}
      for {set f 7} {$n >= (1<<$f)} { incr f 7 } {}
      while {$f > 7} { incr f -7; em1b [expr {($n >> $f) & 0x7f}] }
      em1b [expr {($n & 0x7f) + 128}]
    }
  }

  vopdef as          {v cmd}  { interp alias {} $cmd {} view $v; return $v }
  vopdef asview    + {l args} { view $args vdef $l }
  vopdef blocked   + {v}      { view $v get * 0 | vflat 2 }
  vopdef clone     + {v}      { view $v vstep 0 0 1 1 }
  vopdef collect     {v expr} { uplevel 1 [list view $v loop -collect $expr] }
  vopdef concat    + {v w}    { vcompat $v $w; view [list $v $w] vflat 0 }
  vopdef counts      {v c}    { view $v collect {[vget $v $(#) $c #]} }
  vopdef first     + {v n}    { view $v vstep 0 $n 1 1 }
  vopdef freeze      {v}      { view $v save | load }
  vopdef index       {v cond} { uplevel 1 [list view $v loop -index $cond] }
  vopdef join      + {v w j}  { set c [vcommon $v $w]; vjoin $v $c $w $c $j }
  vopdef join_i    + {v w}    { view $v join $w _J | ungroup -1 }
  vopdef last      + {v n}    { view $v take [expr {-$n}] | reverse }
  vopdef lookup      {v w}  { vcompat $v $w; lindex [vlerq::vgroup dl $w $v] 1 }
  vopdef mapcols   + {v args} { optcrm $v $args }
  vopdef meta      + {v}      { vget $v @ }
  vopdef names       {v}      { vget $v @ * 0 }
  vopdef pair      + {v args} { view [linsert $args 0 $v] vflat -1 }
  vopdef pick      + {v n}    { view $v where {[lindex $n $(#)] != 0} }
  vopdef project   + {v args} { view [optcrm $v $args] | unique }
  vopdef repeat    + {v n}    { view $v vstep 0 [expr {$n*[vget $v #]}] 1 1 }
  vopdef size        {v}      { vget $v # }
  vopdef sort      + {v args} { view $v remap [vsort $v $args] }
  vopdef spread    + {v n}    { view $v vstep 0 [vget $v #] $n 1 }
  vopdef struct_mk   {v}      { vdesc [vget $v @] }
  vopdef structure   {v}      { vstruct [vget $v @] }
  vopdef tag       + {v c}    { view $v pair [view ${c}:I vdef [vget $v * #]] }
  vopdef to          {v var}  { uplevel 1 [list set $var $v] }
  vopdef types       {v}      { vget $v @ * 1 }
  vopdef union     + {v w}    { view $v concat [view $w except $v] }
  vopdef unique    + {v args} { view $v remap [vuniqmap $v $args] }
  vopdef uniquemap   {v args} { vuniqmap $v $args }
  vopdef use         {v w}    { return $w }
  vopdef vdef      + {m args} { view $m mdef | vdata $args }
  vopdef where       {v cond} { uplevel 1 [list view $v loop -where $cond] }
  vopdef width       {v}      { vget $v @ # }

  vopdef except + {v w} {
    vcompat $v $w
    view $v remap [lindex [vlerq::vgroup x $w $v] 1]
  }
  vopdef group + {v args} {
    set gcol [lindex $args end]
    if {$gcol eq ""} { error "group column name missing" }
    set gnames [lrange $args 0 end-1]
    set g0 [optcrm $v $gnames]
    set g1 [optcrm $v [linsert $gnames 0 -omit]]
    set groups {}
    foreach {gm em fm mm} [vlerq::vgroup f $g0 $g0 $g1] break
    set gg [view {} vdata [list $gcol V [list [vget $g1 @]]]]
    view $g0 remap $gm | pair [view $gg vdata [list $mm]]
  }
  vopdef intersect + {v w} {
    vcompat $v $w
    view $v remap [lindex [vlerq::vgroup i $w $v] 1]
  }
  vopdef product + {v w} {
    view $v spread [vget $w #] | pair [view $w repeat [vget $v #]]
  }
  vopdef slice + {v o n {s 1}} {
    view $v vstep $o $n 1 $s
  }
  vopdef rename + {v args} {
    set names [vget $v @ * 0]
    foreach {x y} $args {
      lset names [view $v colnum [list $x]] $y
    }
    set nmeta [view {} vdata [list $names [vget $v @ * 1] [vget $v @ * 2]]]
    view $nmeta vdata [view $v width | collect { [vget $v * $(#)] }]
  }
  vopdef reverse + {v} {
    set n [vget $v #]
    view $v vstep [expr {$n-1}] $n 1 -1
  }
  vopdef take + {v n} {
    if {$n < 0} {
      set v [view $v reverse]
      set n [expr {-$n}]
    }
    view $v vstep 0 $n 1 1
  }
  vopdef transpose + {v {p x}} {
    set n [vget $v #]
    foreach {pn pt} [split ${p}:S :] break
    set m [view $v size | collect { "$pn$(#):$pt" }]
    # FIXME: "get @" works, but "meta" doesn't (because it's lazy?)
    # set c [view $v get @ | collect { [vget $v * $(#)] }]
    set c [view $v width | collect { [vget $v * $(#)] }]
    view $m vdef [eval [linsert $c 0 concat]]
  }
  vopdef ungroup + {v g} {
    set g [view $v colnum $g] ;# force g to be an int index
    foreach {w map} [view $v get * $g | vflat 1 [vget $v @ $g 2]] break
    view $v mapcols -omit $g | remap $map | pair $w
  }

  vopdef max {v c {s ""}} {
    if {$s eq ""} {
      if {[vget $v #] == 0} return
      set s [vget $v 0 $c]
    }
    view $v loop { if {$($c) > $s} { set s $($c) } }
    return $s
  }
  vopdef min {v c {s ""}} {
    if {$s eq ""} {
      if {[vget $v #] == 0} return
      set s [vget $v 0 $c]
    }
    view $v loop { if {$($c) < $s} { set s $($c) } }
    return $s
  }
  vopdef prod {v c {s 1}} {
    view $v loop { set s [expr {$($c)*$s}] }
    return $s
  }
  vopdef sum {v c {s 0}} {
    view $v loop { set s [expr {$($c)+$s}] }
    return $s
  }

  vopdef mutable {
    init {v args} {
      linsert $args 0 mutable [vget $v @] $v
    }
    sizer {t m v nr sr sd dr ir iv} {
      return $nr
    }
    getter {t m v nr sr sd dr ir iv r c} {
      # inserted rows
      foreach x $ir y $iv {
        if {$r < $x} break
        set i [expr {$r-$x}]
        set m [vget $y #]
        if {$i < $m} {
          return [vget $y $i $c]
        }
        set r [expr {$r-$m}]
      }
      # deleted rows
      foreach {x y} $dr {
        if {$r < $x} break
        incr r $y
      }
      # changed values
      set n [lsearch -integer [lindex $sr $c] $r]
      if {$n >= 0} {
        return [lindex $sd $c $n]
      }
      # original data
      vget $v $r $c
    }
  }
  vopdef vorig {v} {
    if {[lindex [vlerq::vinfo $v] 3] eq "view"} { 
      set v [lindex [vlerq::vinfo o $v] 7]
    }
    return $v
  }
  proc vmodify {vname cmd row args} {
    upvar 1 $vname v
    set v [eval [linsert $args 0 view $v $cmd $row]]
  }
  proc mutop {op v r args} {
    set w $v
    set trail {}
    while 1 {
      set o [view $v vorig]
      switch -- [lindex $o 0] {
        at {
          set atrow [lindex $o 2]
          set atcol [lindex $o 3]
          set trail [linsert $trail 0 $atrow $atcol]
          set v [eval [linsert $args 0 view $v m$op $r]]
          set args [list $atcol $v]
          set op set
          set r $atrow
        }
        ref {
          if {[llength $o] == 2} {
            set o [lreplace $o 0 0 ratcl::vmodify]
          } else {
            set o [linsert [lrange $o 2 end] end [lindex $o 1]]
          }
          uplevel 1 [concat $o [list $op $r] $args]
          return $w
        }
        default {
          break
        }
      }
    }
    set result [eval [linsert $args 0 view $v m$op $r]]
    foreach {r c} $trail {
      set result [list at $result $r $c] 
    }
    return $result
  }
  vopdef set {v r args} {
    set n [llength $args]
    if {$n == 0 || $n % 2} { error "incorrect number of arguments" }
    uplevel 1 [linsert $args 0 ratcl::mutop set $v $r]
  }
  vopdef insert {v r w} {
    uplevel 1 [list ratcl::mutop insert $v $r $w]
  }
  vopdef delete {v r n} {
    uplevel 1 [list ratcl::mutop delete $v $r $n]
  }
  proc mut_init {v args} {
    set orig [view $v vorig]
    if {[lindex $orig 0] ne "mutable"} {
      set cols [view $v meta | collect {""}]
      set orig [list mutable $v [vget $v #] $cols $cols {} {} {}]
    }
    upvar 1 _n _n _d _d _v _v
    set _n $args
    set _d $orig
    set _v $v
    uplevel 1 {
      foreach $_n $_d break
    }
  }
  proc mut_done {} {
    uplevel 1 {
      foreach _x $_n { lappend _r [set $_x] }
      set _r
    }
  }
  vopdef mset {v r args} {
###puts [list mset $v $r $args]
    mut_init $v ty bv nr sr sd dr ir iv
    # TODO: deal with inserted/deleted rows
    foreach {x y} $args {
      set c [view $v colnum [list $x]]
      set o 0
      set x -1
      foreach x [lindex $sr $c] {
        if {$r <= $x} break
        incr o
      }
      if {$r == $x} {
        lset sr $c $o $r
        lset sd $c $o $y
      } else {
        if {$r > $x} { set o end }
        lset sr $c [linsert [lindex $sr $c] $o $r]
        lset sd $c [linsert [lindex $sd $c] $o $y]
      }
    }
    mut_done
  }
  vopdef minsert {v r w} {
    mut_init $v ty bv nr sr sd dr ir iv
    # TODO: multiple insertions, careful with overlap
    vcompat $v $w
    incr nr [vget $w #]
    lappend ir $r
    lappend iv $w
    mut_done
  }
  vopdef mdelete {v r n} {
    mut_init $v ty bv nr sr sd dr ir iv
    # TODO: deal with inserted rows
    set nr [expr {$nr - $n}]
    lappend dr $r $n
    mut_done
  }

  vopdef do {w cmds} {
    upvar 1 _v v
    set v $w
    set x ""
    foreach c [split $cmds \n] {
      append x $c "\n                     "
      if {![info complete $x]} continue
      set x [string trim $x]
      regsub {^#.*} $x {} x
      if {$x eq ""} continue
      set cm "view \$_v $x"
      set v [uplevel 1 $cm]
      set x ""
    }
    set w $v
    unset v
    return $w
  }
  vopdef debug {w cmds} {
    upvar 1 _v v
    set v $w
    puts " rows-in  col  msec  view-operation"
    puts " -------  ---  ----  --------------"
    set x ""
    set w "                    "
    foreach c [split $cmds \n] {
      append x $c \n $w
      if {![info complete $x]} continue
      set x [string trim $x]
      if {[regexp {^#} $x]} {
        puts [format {%20s %.58s} "" $x]
        set x ""
      }
      switch -- $x {
        "" { }
        ?  { puts "$w ?\n[view $v dump]" }
        default {
          if {[catch { view $v size } nr]} { set nr "" }
          if {[catch { view $v width } nc]} { set nc "" }
          set cm "view \$_v $x"
          set us [lindex [time {
            set v [uplevel 1 $cm]
            catch {view $v size}
          }] 0]
          puts [format {%8s %4s %5.0f  %s} $nr $nc [expr {$us/1000.0}] $x]
        }
      }
      set x ""
    }
    if {[catch { view $v size } nr]} { set nr " -------" }
    if {[catch { view $v width } nc]} { set nc " ---" }
    puts [format {%8s %4s  ----  --------------} $nr $nc]
    set w $v
    unset v
    return $w
  }

  vopdef open {path} {
    view $path mapf
  }
  vopdef chan {fd} {
    fconfigure $fd -translation binary
    set bytes [read $fd 8]
    binary scan $bytes @4I remain
    if {$remain > 0} {
      append bytes [read $fd $remain]
    } else {
      append bytes [read $fd]
    }
    list load $bytes
  }
  vopdef save {v {type ?} {chan ?}} {
    if {$type eq "?"} { ;# string
    	save::asmk $v
    } elseif {$chan eq "?"} { ;# file: open and close temp channel
    	set fd [::open $type w]
    	fconfigure $fd -translation binary
    	puts -nonewline $fd [save::asmk $v]
    	close $fd
    	file size $type
    } else { ;# channel: basic case
    	if {$type ne "-to"} { error "expected '-to' as first arg" }
    	fconfigure $chan -translation binary
    	set data [save::asmk $v]
    	puts -nonewline $chan $data
    	string length $data
    }
  }
  vopdef dump {vid {maxrows 20}} {
    set n [view $vid get #]
    if {[view $vid width] == 0} { return "  ($n rows, no columns)" }
    set i -1
    if {$n > $maxrows} { set vid [view $vid first $maxrows] }
    foreach x [view $vid names] y [view $vid types] {
      set c [view $vid get * [incr i]]
      if {$x eq ""} { set x $i }
      switch $y {
      	B {
      	  set r {}
      	  foreach z $c { lappend r "[string length $z]b" }
      	  set c $r
      	}
      	V {
      	  set r {}
      	  #foreach z $c { lappend r "#[view $z get #]" }
      	  foreach z $c {
      	    set zz [vget $z #]
      	    lappend r "#$zz"
      	  }
      	  set c $r
      	}
      }
      set w [string length $x]
      foreach z $c {
      	if {[string length $z] > $w} { set w [string length $z] }
      }
      if {$w > 50} { set w 50 }
      switch $y {
      	B - I - L - F - D - V { append fmt "  " %${w}s }
      	default               { append fmt "  " %-$w.${w}s }
      }
      append hdr "  " [format %-${w}s $x]
      append bar "  " [string repeat - $w]
      lappend d $c
    }
    set r [list $hdr $bar]
    for {set i 0} {$i < $n} {incr i} {
      if {$i >= $maxrows} break
      set cmd [list format $fmt]
      foreach x $d { lappend cmd [regsub -all {[^ -~]} [lindex $x $i] .] }
      lappend r [eval $cmd]
    }
    if {$i < $n} { lappend r [string map {- .} $bar] }
    ::join $r \n
  }
  vopdef html {vid} {
    set names [view $vid names]
    set types [view $vid types]
    set o <table>
    append o {<style type="text/css"><!--\
      table {border-spacing:0; border:1px solid #aaa; margin:0 0 2px 0}\
      th {background-color:#eee; font-weight:normal}\
      td {vertical-align:top}\
      th,td {padding:0 2px}\
      th.row,td.row {color:#aaa; font-size:75%}\
    --></style>}
    append o \n {<tr><th class="row"></th>}
    foreach x $names { append o <th> $x </th> }
    append o </tr>\n
    view $vid loop c {
      append o {<tr><td align="right" class="row">} $c(#) </td>
      foreach x $names y $types {
      	switch $y {
      	  b - B	  { set z [string length $c($x)]b }
      	  v - V	  { set z [view $c($x) html] }
      	  default { set z [string map {& &amp\; < &lt\; > &gt\;} $c($x)] }
      	}
      	switch $y {
      	  s - S - v - V { append o {<td>} }
      	  default	{ append o {<td align="right">} }
      	}
      	append o $z </td>
      }
      append o </tr>\n
    }
    append o </table>\n
  }
}

#parray vops
