# Relational algebra for Tcl

package provide ratcl 1.0
package require -exact thrill 1.0

namespace eval ::thrill {
  namespace export vdef vfun view vsave

  namespace eval v {
    variable init2    ;# more Thrill init code is loaded from a separate file
    variable funcs    ;# additional info for defined functions
    variable mkseq 0  ;# sequence number for Mk4tcl db names
    variable egram    ;# expression grammar tables
    variable epars    ;# expression parser object
  }

  if {![info exists v::init2]} { set v::init2 [readfile ../noarch/words2.th] }
  if {![info exists v::egram]} { set v::egram [readfile ../noarch/expr.db] }

  # make sure the workspace has been set up, then load additional Thrill code
  if {![info exists v::ws]} vsetup
  vscript $v::init2

  # set up the expression parser object
  set v::epars [vcall vparini B(S)II $v::egram {<= >= != == && ||} 2 0]

# this redefines the dummy definition in thrill.tcl to look up mutable views
  proc vwfix {vid} {
    vcall vsget V $vid
  }
# construct a view from a description and Tcl-supplied data
  proc vdef {args} {
    if {![info exists v::ws]} vsetup
    set data [lindex $args end]
    set args [lrange $args 0 end-1]
    set d [llength $data]
    set c [llength $args]
    if {$c == 0} {
      if {![string is int -strict $data]} { error "no columns, count expected" }
      return [vcall mkcount I $data]
    }
    if {$d > 0} {
      if {$c == 0} { error "no args defined" }
      if {$d%$c != 0} { error "data is not an exact number of rows" }
      set n [expr {$d/$c}]
    } else {
      set n 0
    }
    set mdata {}
    set desc ""
    set subs {}
    set i 0
    foreach a $args {
      if {[llength $a] == 2} {
	set t V
	set x [lindex $a 0]
	set a [concat vdef [lindex $a 1] [list {}]]
	lappend subs $i [eval $a]
	for {set r 0} {$r < $n} {incr r} {
	  set p [expr {$i+$r*$c}]
	  lset data $p [eval [lset a end [lindex $data $p]]]
	}
      } else {
	set t S
	foreach {x y} [split $a :] break
	if {$y ne ""} { set t $y }
	if {$t eq "V"} { lappend subs $i [lindex $data $i] }
      }
      lappend mdata $x [expr {[scan $t %c] & 63}] $v::mnocol
      append desc [string map {B S} $t]
      incr i
    }
    # if input data contains views, make sure they are the current version
    foreach {i v} $subs {
      for {set r 0} {$r < $n} {incr r} {
	set p [expr {$i+$r*$c}]
	lset data $p [vwfix [lindex $data $p]]
      }
    }
    # first create a meta row, then use it to define the actual root view
    set r [vcall mkvdef I($desc)(SIV) $n $data $mdata]
    # set metaview in parent for each subview
    foreach {i v} $subs {
      # careful if we have no view to use (i.e. args has *:V col but no data)
      if {$v ne ""} { vcall { fixvdef 0 } VIV $r $i $v }
    }
    return $r
  }
# define a new function, or get its definition
  proc vfun {{name ""} {type S} args} {
    if {$name eq ""} { return [array names v::funcs] }
    if {[llength $args]} { set v::funcs($name) [list $type $args] }
    return $v::funcs($name)
  }
# interface to the expression parser
  proc exparse {str} {
    vcall parser VS $v::epars $str
  }
# process a view object call, including pipelines
  proc view {vid args} {
    while {[llength $args]} {
      set n [lsearch -exact $args |]
      if {$n < 0} { set n [llength $args]; lappend args | }
      set v [lreplace $args $n end]
      if {[llength $v]} {
	set cmd [lreplace $v 0 0 ::thrill::m_[lindex $v 0] [vwfix $vid]]
	set vid [uplevel 1 $cmd]
      }
      set args [lreplace $args 0 $n]
    }
    return $vid
  }
# multi-line utility command: view ... | do { ...\n...\n... } | ...
  proc m_do {vid cmds} {
    variable do_vid $vid
    set r {::thrill::view $::thrill::do_vid}
    foreach x [split $cmds \n] {
      if {![regexp {^\s*#} $x]} { append r " | " $x }
    }
    uplevel 1 $r
  }
# Cursors tied to a view and looping
  proc rtracer {a e op} {
    if {$e ne "^" && $e ne "#"} {
      upvar 1 $a aref
      set aref($e) [m_get $aref(^) $aref(#) $e]
    }
  }
  proc wtracer {a e op} {
    if {$e ne "^" && $e ne "#"} {
      upvar 1 $a aref
      uplevel 1 [list ::thrill::m_set $aref(^) $aref(#) $e $aref($e)]
    }
  }
  proc m_cursor {vid aname} {
    upvar 1 $aname aref
    unset -nocomplain aref
    set aref(^) $vid
    set aref(#) 0
    foreach x [vcall ^names V $vid] {
      set aref($x) ""
    }
    trace add variable aref read ::thrill::rtracer 
    trace add variable aref write ::thrill::wtracer 
    return $vid
  }
  proc m_each {vid aname body} {
    uplevel 1 [list ::thrill::m_cursor $vid $aname]
    upvar 1 $aname aref
    set n [vcall ^count V $vid]
    for {set i 0} {$i < $n} {incr i} {
      set aref(#) $i
      set c [catch { uplevel 1 $body } r]
      switch -exact -- $c {
        0 {}
	1 { return -errorcode $::errorCode -code error $r }
	3 { return }
	4 {}
	default { return -code $c $r }
      }
    }
  }
# present the contents of a view in a nice format
  proc m_dump {vid {maxrows 20}} {
    set nm [vcall ^names V $vid]
    if {[llength $nm] == 0} return
    set i -1
    set n [vcall ^count V $vid]
    if {$n > $maxrows} { set vid [m_first $vid $maxrows] }
    foreach x $nm y [vcall ^types V $vid] {
      set c [vcall getcol Va $vid [incr i]]
      if {$x eq ""} { set x $i }
      switch $y {
	b -
	B {
	  set r {}
	  foreach z $c { lappend r "[string length $z]b" }
	  set c $r
	}
	v -
	V {
	  set r {}
	  foreach z $c { lappend r "#[vcall ^count V $z]" }
	  set c $r
	}
      }
      set w [string length $x]
      foreach z $c {
	if {[string length $z] > $w} { set w [string length $z] }
      }
      if {$w > 50} { set w 50 }
      switch $y {
	b - B - i - I - v - V { append fmt "  " %${w}s }
	default   { append fmt "  " %-$w.${w}s }
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
    join $r \n
  }
# render a view as html
  proc m_html {vid} {
    set names [vcall ^names V $vid]
    set types [vcall ^types V $vid]
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
    m_each $vid c {
      append o {<tr><td align="right" class="row">} $c(#) </td>
      foreach x $names y $types {
	switch $y {
	  b - B	  { set z [string length $c($x)]b }
	  v - V	  { set z [m_html $c($x)] }
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
# set up an alias for a view
  proc m_as {vid name} {
    interp alias {} $name {} ::thrill::view $vid
    return $vid
  }
# generate a calculated column
  proc m_calc {vid name ex} {
    vdef $name [uplevel 1 [list ::thrill::vcall genc IVVS 1 $vid $v::epars $ex]]
  }
# extend a view with a calculated column
  proc m_extend {vid args} {
    if {[llength $args] % 1} { error "even number of arguments expected" }
    foreach {name ex} $args {
      set vid [m_pair $vid [uplevel 1 [list ::thrill::m_calc $vid $name $ex]]]
    }
    return $vid
  }
# filter on condition parsed at runtime, returns row map
  proc m_ifmap {vid ex} {
    uplevel 1 [list ::thrill::vcall genc IVVS 0 $vid $v::epars $ex]
  }
# where uses ifmap and maprow
  proc m_where {vid ex} {
    m_maprow $vid [uplevel 1 [list ::thrill::m_ifmap $vid $ex]]
  }
# access to Tcl variables and fixed array elements
  proc varget {r} {
    regexp {^\$(\w+)\(?(\w*)\)?:?(\w)?$} $r - y z t
    if {$z ne ""} { append y ( $z ) }
    # skip an extra level due to the eval in vcall
    upvar 2 $y a
    if {$t eq ""} { set t S }
    uplevel 1 set t $t ;# messes with the caller, i.e. vcall!
    return $a
  }
# function callbacks
  proc funget {fun argv} {
    foreach {typ cmd} $v::funcs($fun) break
    set r [uplevel 2 $cmd $argv]
    uplevel 1 set t $typ ;# messes with the caller, i.e. vcall!
    return $r
  }
# general information
  proc m_info {vid {type #}} {
    switch -- $type {
      "#"     { foreach x {args arity names tag types width} {
		  lappend r $x [m_info $vid $x]
		}
	      }
      args    { vcall ^args V $vid }
      arity   { vcall ^arity V $vid }
     #cols    { vcall ^info V $vid }
      names   { vcall ^names V $vid }
      tag     { vcall ^type V $vid }
      types   { vcall ^types V $vid }
      width   { vcall ^width V $vid }
      default { error "unknown type '$type', must be omitted or one of:\
			args, arity, names, tag, types, or width" }
    }
  }
# xml tokenizer
  proc xmltokens {str} {
    vcall xmltokens S $str
  }
# xml tokenizer loop
  proc xmlscan {str args} {
    # $args is accessed via an upvar in xmlfun (!)
    vcall { xmloop Wn } S $str
  }
# xml tokenizer callback
  proc xmlfun {args} {
    upvar 2 args cb
    uplevel 2 $cb $args
  }
# set one or more values in a row
  proc m_set {vid row args} {
    tracer
    if {[llength $args] % 2} { error "not even count of col/value pairs" }
    if {$row < 0} { incr row [vcall ^count V $vid] }
    set cols {}
    set vals {}
    foreach {x y} $args { lappend cols $x; lappend vals $y }
    set vid [vcall ^Mut V $vid]
    foreach x $cols y $vals z [vcall ctypes V(a) $vid $cols] {
      set z [string map {2 B 9 I 19 S 22 V 34 b 41 i 51 s 54 v } $z]
      vcall ^vset VIa$z $vid $row $x $y
    }
    return $vid
  }
# group uses last arg as new column name
  proc m_group {v args} {
    vcall ^group V(a)S $v [lrange $args 0 end-1] [lindex $args end]
  }
# save to string, file, or channel
  proc m_save {vid args} {
    foreach {x y} $args break
    switch [llength $args] {
      0 { ;# string: via memchan
	set fd [do_memchan]
	fconfigure $fd -translation binary
	vcall ^save VS $vid $fd
	seek $fd 0
	set s [read $fd]
	close $fd
	return $s
      }
      1 { ;# file: open and close temp channel
	set fd [open $x w]
	fconfigure $fd -translation binary
	set n [vcall ^save VS $vid $fd]
	close $fd
	return $n
      }
      2 { ;# channel: basic case
	if {$x ne "-to"} { error "expected '-to' as first arg" }
	fconfigure $y -translation binary
	return [vcall ^save VS $vid $y]
      }
      default { error "too many arguments" }
    }
    return $n
  }
# save to string, file, or channel - new version
  proc vsave {vid args} {
    foreach {x y} $args break
    switch [llength $args] {
      0 { ;# string: via memchan
	set fd [do_memchan]
	fconfigure $fd -translation binary
	vcall ^save2 VS $vid $fd
	seek $fd 0
	set s [read $fd]
	close $fd
	return $s
      }
      1 { ;# file: open and close temp channel
	set fd [open $x w]
	fconfigure $fd -translation binary
	set n [vcall ^save2 VS $vid $fd]
	close $fd
	return $n
      }
      2 { ;# channel: basic case
	if {$x ne "-to"} { error "expected '-to' as first arg" }
	fconfigure $y -translation binary
	return [vcall ^save2 VS $vid $y]
      }
      default { error "too many arguments" }
    }
    return $n
  }
# interface to the memchan package
  proc do_memchan {} {
    if {[catch { package require vfslib; ::vfs::memchan } fd]} {
      package require Memchan
      set fd [::memchan]
    }
    return $fd
  }
# open a metakit data file, overwrites the proc in thrill.tcl, extended api
  proc vopen {type {arg ""}} {
    if {![info exists v::ws]} vsetup
    if {$arg eq ""} { set arg $type; set type -file }
    switch -- $type {
      -data { ;# string
        return [vcall mkopen B $arg]
      }
      -from { ;# channel: reads to eof
	fconfigure $arg -translation binary
        return [vcall mkopen B [read $arg]]
      }
      -file { ;# file
	set c [catch { vcall mkfopen S $arg } r]
	if {$c == 1} { set r "$arg: cannot open (error $r)" }
	return -code $c $r
      }
    }
    error "expected -data, -from, or a filename"
  }

  proc m_blocked   {v}       { vcall ^blocked   V     $v             }
  proc m_clone     {v}       { vcall ^clone     V     $v             }
  proc m_concat    {v a}     { vcall ^concat    VV    $v $a          }
  proc m_counts    {v a}     { vcall ^counts    Va    $v $a          }
  proc m_delete    {v a b}   { vcall ^delete    VII   $v $a $b       }
  proc m_describe  {v}       { vcall ^describe  V     $v             }
  proc m_divide    {v a}     { vcall ^divide    VV    $v $a          }
  proc m_except    {v a}     { vcall ^except    VV    $v $a          }
  proc m_first     {v a}     { vcall ^first     VI    $v $a          }
  proc m_freeze    {v}       { vcall ^freeze    V     $v             }
  proc m_intersect {v a}     { vcall ^intersect VV    $v $a          }
  proc m_join      {v a b}   { vcall ^join      VVS   $v $a $b       }
  proc m_join0     {v a}     { vcall ^join0     VV    $v $a          }
  proc m_join_i    {v a}     { vcall ^join_i    VV    $v $a          }
  proc m_join_l    {v a}     { vcall ^join_l    VV    $v $a          }
  proc m_last      {v a}     { vcall ^last      VI    $v $a          }
  proc m_mapcols   {v args}  { vcall ^mapcols   V(a)  $v $args       }
  proc m_maprow    {v a}     { vcall ^maprow    V(I)  $v $a          }
  proc m_meta      {v}       { vcall ^meta      V     $v             }
  proc m_nspread   {v a}     { vcall ^nspread   V(I)  $v $a          }
  proc m_pair      {v a}     { vcall ^pair      VV    $v $a          }
  proc m_pass      {v}       { vcall ^pass      V     $v             }
  proc m_pick      {v a}     { vcall ^pick      V(I)  $v $a          }
  proc m_product   {v a}     { vcall ^product   VV    $v $a          }
  proc m_project   {v args}  { vcall ^project   V(a)  $v $args       }
  proc m_rename    {v args}  { vcall ^rename    V(aS) $v $args       }
  proc m_repeat    {v a}     { vcall ^repeat    VI    $v $a          }
  proc m_reverse   {v}       { vcall ^reverse   V     $v             }
  proc m_slice     {v a b c} { vcall ^slice     VIII  $v $a $b $c    }
  proc m_sort      {v args}  { vcall ^sort      V(a)  $v $args       }
  proc m_spread    {v a}     { vcall ^spread    VI    $v $a          }
  proc m_subcat    {v a}     { vcall ^subcat    Va    $v $a          }
  proc m_tag       {v a}     { vcall ^tag       VS    $v $a          }
  proc m_take      {v a}     { vcall ^take      VI    $v $a          }
  proc m_type      {v}       { vcall ^type      V     $v             }
  proc m_ungroup   {v a}     { vcall ^ungroup   Va    $v $a          }
  proc m_union     {v a}     { vcall ^union     VV    $v $a          }
  proc m_uniqmap   {v}       { vcall ^uniqmap   V     $v             }
  proc m_unique    {v args}  { vcall ^unique    V(a)  $v $args       }

  proc m_append    {v a}     { tracer; vcall ^append  VV  $v $a      }
  proc m_insert    {v a b}   { tracer; vcall ^insert  VIV $v $a $b   }
  proc m_delete    {v a b}   { tracer; vcall ^delete  VII $v $a $b   }
  proc m_replace   {v a b}   { tracer; vcall ^replace VIV $v $a $b   }

  interp alias {} ::thrill::m_get {} ::thrill::vget 

# the following code deals with changes by generating Metakit commands
  namespace eval v {
    variable names	  ;# cache, maps named view to column names
    variable commands {}  ;# collects MK commands to be executed for a commit
  }
# this intercepts all modifying calls: set, append, delete, insert, replace
  proc tracer {} {
    set args [info level -1]
    eval [lreplace $args 0 0 x_[string range [lindex $args 0] 12 end]]
  }
  proc m_mk4name {db name} {
    set vid [view $db get 0 $name]
    set path db.$name
    vcall vpdef nSV "" $path $vid
    set v::names($path) [view $vid info names]
    return $vid
  }
  proc m_commit {vid filename} {
    package require Mk4tcl
    if {[llength $v::commands]} {
      set db [lindex [split [lindex $v::commands 0 2] .] 0]
      mk::file open $db $filename -nocommit
      foreach c $v::commands { eval $c }
      mk::file commit $db ;# only commit if there were no errors
      mk::file close $db
    }
    set v::commands {}
  }
# routines called to apply changes via equivalent Mk4tcl calls
  proc x_set {vid row args} {
    set a [vcall vpath V $vid]
    if {$a eq ""} {
      set a $vid?
      set n [view $vid info names]
    } else {
      set n $v::names($a)
    }
    set c [list mk::set $a!$row]
    foreach {x y} $args {
      if {[string is int -strict $x]} { set x [lindex $n $x] }
      lappend c $x $y
    }
    lappend v::commands $c
  }
  proc x_append {vid vid2} {
    x_insert $vid [view $vid get #] $vid2
  }
  proc x_delete {vid row cnt} {
    set a [vcall vpath V $vid]
    if {$a eq ""} { set a $vid? }
    lappend v::commands [list mk::row delete $a!$row $cnt]
  }
  proc x_insert {vid row vid2} {
    set a [vcall vpath V $vid]
    if {$a eq ""} {
      set a $vid?
      set n [view $vid info names]
    } else {
      set n $v::names($a)
    }
    lappend v::commands [list mk::row insert $a!$row [view $vid2 get #]]
    view $vid2 each r {
      set c [list mk::set $a!$row]
      foreach x $n {
	lappend c $x $r($x)
      }
      lappend v::commands $c
      incr row
    }
  }
  proc x_replace {vid row vid2} {
    x_delete $vid $row [view $vid2 get #]
    x_insert $vid $row $vid2
  }
}

namespace eval ::ratcl {
  namespace import ::thrill::*
  namespace export vsetup vfinish vopen vget vdef vfun view vsave
}
