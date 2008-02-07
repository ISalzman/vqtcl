# ratcl.tcl -- Relational algebra ond other utility operators for vlerq

package provide ratcl 4
package require vlerq 4

namespace eval vlerq {}
interp alias {} view {} vlerq view

namespace eval ratcl {
  namespace export vopdef
  
  proc vopdef {name params body} {
    proc ::vlerq::$name $params $body
  }

  vopdef as      {v cmd}  { interp alias {} $cmd {} view $v; return $v }
  vopdef collect {v expr} { uplevel 1 [list view $v loop -collect $expr] }
  vopdef counts  {v c}    { view $v loop -collect {[view $($c) size]} }
  vopdef freeze  {v}      { view $v emit | load }
  vopdef index   {v cond} { uplevel 1 [list view $v loop -index $cond] }
  vopdef pick    {v n}    { view $v where {[lindex $n $(#)] != 0} }
  vopdef use     {v w}    { return $w }
  vopdef where   {v cond} { uplevel 1 [list view $v loop -where $cond] }

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
            set zz [vlerq get $z #]
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
          b - B   { set z [string length $c($x)]b }
          v - V   { set z [view $c($x) html] }
          default { set z [string map {& &amp\; < &lt\; > &gt\;} $c($x)] }
        }
        switch $y {
          s - S - v - V { append o {<td>} }
          default { append o {<td align="right">} }
        }
        append o $z </td>
      }
      append o </tr>\n
    }
    append o </table>\n
  }

  vopdef transpose {v {p x}} {
    set n [vlerq size $v]
    foreach {pn pt} [split ${p}:S :] break
    set m [view $v size | collect { "$pn$(#):$pt" }]
    set c [view $v get @ | collect { [vlerq get $v * $(#)] }]
    view $m def [eval [linsert $c 0 concat]]
  }

  vopdef freq {v i} {
    set n [view $v width | iota]
    set g [vlerq group $v $n _]
    set c [vlerq loop $g -collect { [view $(_) size] }]
    view $g colmap $n | pair [view ${i}:I def $c]
  }
}
