#! /usr/bin/env tclkit
# vide.tcl - Vlerq IDE

package require Tk

proc initGui {{geom ""}} {
  switch -glob $::tcl_platform(platform),[tk windowingsystem] {
    windows,*   { set font {Courier 9} }
    macintosh,* { set font {Monaco 9} }
    *,aqua      { set font {Monaco 9} }
    default     { set font {fixed 10} }
  }
  if {[tk windowingsystem] eq "aqua"} {
    event delete <<Cut>> <Key-F2>
    event delete <<Copy>> <Key-F3>
    event delete <<Paste>> <Key-F4>
  }

  wm title . "VIDE - [pwd]"
  if {$geom eq ""} { set geom +550+30 } ;# just 'cause it's convenient for me
  wm geometry . $geom
  . configure -bg #eee

  bind . <Escape> {
    if {[lindex $argv 0] eq "-vide"} {
      exec [info nameofexe] $argv0 -vide [wm geometry .] &
    } else {
      exec [info nameofexe] $argv0 [wm geometry .] &
    }
    exit
  }
  foreach x {Shift-F5 Command-q Control-q} {
    bind . <$x> exit
  }

  option add *Text.highlightThickness 1
  option add *Text.highlightColor #888
  option add *Text.borderWidth 0
  option add *Text.font $font
  option add *Text.takeFocus 0

  option add *Entry.highlightThickness 1
  option add *Entry.highlightColor #888
  option add *Entry.borderWidth 0
  option add *Entry.font $font
  
  text .o -width 80 -height 25 -state disabled
  text .v -width 30
  text .i -width 1 -height 1 -wrap word
  text .m -width 1 -height 1
  text .h -width 1 -height 5 -wrap word
  text .d -width 60 -height 1 -wrap none
  entry .c -background white

  .o tag configure red -foreground red
  .o tag configure blue -foreground blue
  .h tag configure rjust -justify right
  .d tag configure rjust -justify right

  foreach {w c} {
    .d #fff4f4 .h #fffff4 .v #f4f4ff .i #f4ffff .o #f8f8f8 .m #eee
  } {
    $w configure -background $c -highlightbackground $c 
  }

  if {0} {
    # don't show the viewer & mode panes yet, they do nothing at this stage
    grid configure .o -  .v -sticky news -padx 1 -pady 1
    grid configure .h .i ^  -sticky news -padx 1 -pady 1
    grid configure .d .c .m -sticky news -padx 1 -pady 1
    grid rowconfigure . 0 -weight 1
    grid columnconfigure . 2 -weight 1
  } else {
    grid configure .o -  -sticky news -padx 1 -pady 1
    grid configure .h .i -sticky news -padx 1 -pady 1
    grid configure .d .c -sticky news -padx 1 -pady 1
    grid rowconfigure . 0 -weight 1
    grid columnconfigure . 0 -weight 1
  }

  bind .c <Return>        { doCmd \n; break }
  bind .c <space>         { doCmd " "; break }
  focus .c

  .h insert 1.0 "\n\n\n\n\n"
  .m insert 1.0 "(modes)"
  .v insert 1.0 "viewer"
}
proc doCmd {mode} {
  set cmd [.c get]
  .c delete 0 end
  if {$cmd ne "" || $mode eq "\n"} {
    .h insert end "$cmd$mode" rjust
    if {$mode eq "\n"} { .h tag remove rjust 1.0 end }
    if {$cmd ne ""} { doOne $cmd }
  }
  .h see end
}
proc bgerror {msg} {
  puts stderr $msg
}
proc getTop {n} {
  set x [thrill::vcall { pick dup typ swap dup b2i swap 3 pack } I $n]
  lset x 0 [lindex {nil int str dat any obj esc} [lindex $x 0]]
}
proc showStack {} {
  set dsize [thrill::vcall depth]
  .d delete 1.0 end
  for {set i 0} {$i < $dsize} { incr i} {
    foreach {x y z} [getTop $i] break
    switch $x {
      nil { if {$y == 0} { set z <nil> } else { set z <nil,$y> } }
      obj { if {$y == -1} { set z <obj> } else { set z <obj,$y> } }
      any { set z [list $z] }
      str { set z '$z }
    }
    if {[string length $z] > 20} {
      set z [string range $z 0 19]...
    }
    .d insert 1.0 "$z " rjust
    .d see end
    if {[string length [.d get 1.0 end]] > 75} break
  }
}
proc showInfo {} {
  set x [thrill::vcall {
    Wc dup b2i swap tmax
    depth
    Wv dup b2i swap tmax
    Wd work tmax Wr - 2- b2i 
    Ww b2i var0 - dsp0 2- var0 -
    Wt b2i
    Wg dup tlen swap tmax
    Wu Wh
    13 pack
  }]
  .i delete 1.0 end
  .i insert end [eval [linsert $x 0 format \
"code %5d/%-4d d %d
syms %5d/%-4d r %d
vars %5d/%-4d t %d
getf %5d/%-4d
gc%8d/%d"]]
}
proc doOne {cmd} {
  if {$cmd ne ""} {
#_puts -nonewline " $cmd "; flush stdout
    set w $thrill::v::ws
    $w eval $cmd
    while {1} {
      set r [$w run]
      switch -- $r {
	0 -
	1 { break }
	2 { puts -nonewline [$w pop] }
	3 { $w push S [thrill::readfile [$w pop]] }
	4 { $w push S [eval [$w pop]] }
	5 { $w push I [expr {int([clock clicks])}] }
	6 { if {[catch { $w push S $env([$w pop]) }]} { $w push n - } }
	default { error "*** trampoline $r ?" }
      }
    }
#_puts .
  }
  showInfo
  showStack
}
proc emit {tag msg} {
  .o configure -state normal
  .o insert end $msg $tag
  .o see end
  .o configure -state disabled
}

if {[lindex $argv 0] eq "-vide"} {
  initGui [lindex $argv 1]
} else {
  initGui [lindex $argv 0]
}

if {1} {
  rename puts _puts
  proc puts {args} {
    set i 0
    set sep \n
    if {[lindex $args 0] eq "-nonewline"} {
      set sep ""
      incr i
    }
    if {[llength $args] == $i+1 || [lindex $args $i] eq "stdout"} {
      emit "" [lindex $args end]$sep
    } elseif {[lindex $args $i] eq "stderr"} {
      emit red [lindex $args end]$sep
    } else {
      eval _puts $args
    }
  }
  proc tclLog {msg} {
    emit blue $msg\n
  }
}

tclLog {
    Welcome to VIDE, Vlerq's mini IDE to run Thrive and Thrill code.
    The entry field responds the moment you hit <space> or <return>.
    Slowly enter "1 2 + dup * .p " to see what happens, or "words ".
    More: <F1>..<F9> shows the stack, <ctrl-q> exits, <esc> reloads.
}

# initialize vlerq after output has been redirected to the .o pane
load ./thrive[info sharedlibext]
source thrill.tcl
namespace import thrill::*
vsetup

if {[file exists vide.rc]} { tclLog "loading vide.rc"; source vide.rc }

getTop 0 ;# force compilation before use (needed for interactive use of { and })
doOne "" ;# initialize info shown on screen
