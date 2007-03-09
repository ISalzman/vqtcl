# Thrill runtime interface for Tcl

package provide thrill 1.0
package require -exact thrive 2.23

namespace eval ::thrill {
  namespace export vsetup vfinish vopen vget

  namespace eval v {
    variable debug 0  ;# include Thrill debug code & check Thrive calls if set
    variable ws       ;# vm command object once inited, i.e. the workspace
    variable handlers ;# vector to dispatch trampoline callbacks
    variable rcache   ;# array to remember symbol lookups as references
    variable init     ;# Thrill init code is loaded from a separate file
    variable mnocol   ;# meta view of a view with no columns
  }

# access to VFS needs to go through Tcl's I/O system
  proc readfile {fn} {
    set fd [open $fn]
    fconfigure $fd -translation binary
    set data [read $fd]
    close $fd
    return $data
  }

  if {![info exists v::init]} { set v::init [readfile ../noarch/words1.th] }

# define the handlers for each possible trampoline return value
  set v::handlers {
    { return }
    { set t [$w pop]; set s [eval [$w pop]]; $w push $t $s }
    { error "token?" }
    { puts -nonewline [$w pop] }
    { $w push S [readfile [$w pop]] }
    { $w push I [expr {int([clock clicks])}] }
    { if {[catch { $w push S $env([$w pop]) }]} { $w push n - } }
    { set m [$w pop]; set o [$w pop]; set f [$w pop]
      set m [string map {0 start 1 current 2 end} $m]
      seek $f $o $m; $w push I [tell $f] }
    { set i [$w pop]; $w push S [read [$w pop] $i] }
    { set s [$w pop]; puts -nonewline [$w pop] $s; $w push I 0 }
  }

# debug variant of vcall which verifies that the stack remains balanced
  proc vcall {cmd args} {
    set s [tcall depth]
    set r [uplevel 1 [linsert $args 0 ::thrive::tcall $cmd]]
    set t [tcall depth]
    if {$s != $t} {
      error "unbalanced call: $cmd ($s/$t before/after)"
    }
    return $r
  }
# call to the Thrive VM - pushes args, handles call, retrieves result
# to disable stack checks: name this proc "vcall", to enable: name it "tcall"
  proc tcall {cmd {fmt ""} args} {
    set w $v::ws
    eval [linsert $args 0 $w push $fmt]
    if {![info exists v::rcache($cmd)]} {
      if {[string first " " $cmd] >= 0} {
	$w load "{ $cmd }"
	set v::rcache($cmd) [$w pop]
      } else {
	set v::rcache($cmd) [$w find $cmd]
      }
    }
    set req [$w run $v::rcache($cmd)]
    while {$req != -1} {
      eval [lindex $v::handlers $req]
      set req [$w run]
    }
    $w pop
  }
# initialize thrive vm by loading thrill code into it
# this is usually called automatically by the first use of vdef or vopen
  proc vsetup {{argv ""}} {
    if {[info exists ::env(VLERQ_DEBUG)]} {
      set v::debug 1
    } else {
      rename vcall ""
      rename tcall vcall
    }
    set v::ws [::thrive]
    set me ""
    foreach x [info loaded] {
      if {[lindex $x 1] eq "Thrive"} { set me [lindex $x 0] }
    }
    $v::ws push S $me
    $v::ws push (S) $argv
    vscript $v::init
    set v::mnocol [vcall Mvnone]
  }
# compile a script, include D lines in debug mode
  proc vscript {s} {
    if {$v::debug} { set d " " } else { set d "#" }
    foreach x [split $s \n] {
      $v::ws load [regsub {^D } $x $d]
    }
  }
# cleanup
  proc vfinish {} {
    set ds [vcall Wd]
    set d0 [vcall dsp0]
    if {$ds != $d0} { error "data stack not empty ($ds, should be $d0)" }
    rename $v::ws ""
    unset v::ws
  }
# open a metakit data file, this proc will be overwritten in ratcl.tcl
  proc vopen {fn} {
    if {![info exists v::ws]} vsetup
    set c [catch { vcall mkfopen S $fn } r]
    if {$c == 1} {
      # fallback to VFS-aware opening if memory-mapped open failed
      if {$r == -1000} { return [vcall mkopen B [readfile $fn]] }
      set r "$fn: cannot open (error $r)"
    }
    if {$c == 1} { set r "$fn: cannot open (error $r)" }
    return -code $c $r
  }
# general data access routine
  proc vget {vid args} {
    while {[llength $args] > 2} {
      set vid [vwfix [vcall getcell VIa $vid [lindex $args 0] [lindex $args 1]]]
      set args [lrange $args 2 end]
    }
    lappend args * *
    set r [lindex $args 0]
    set c [lindex $args 1]
    switch -- $r {
      # { return [vcall ^count V $vid] }
      * { if {[llength $args] % 2} {
	    set l {}
	    set n [vcall ^count V $vid]
	    for {set i 0} {$i < $n} {incr i} {
	      lappend l [vcall getrow VI $vid $i]
	    }
	    return $l
	  }
	  if {$c eq "*"} { return [vcall getall V $vid] }
	  return [vcall ^getcol Va $vid $c]
	}
    }
    if {$r < 0} { incr r [vcall ^count V $vid] }
    if {$c eq "*"} {
      return [vcall getrow VI $vid $r]
    }
    vcall getcell VIa $vid $r $c
  }
# dummy proc to deal with mutable state, this is redefined in ratcl.tcl
  proc vwfix {vid} {
    return $vid
  }
}
