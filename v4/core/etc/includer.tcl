#!/usr/bin/env tclkit
# includer -=- expand includes into source files

if {[llength $argv] < 1} {
  puts stderr "Usage: $argv0 file ?omit...? >result"
  exit 1
}

proc expand {fn} {
  global files seen 
  set f [file tail $fn]
  if {![info exists files($f)]} {
    return 0
  }
  if {![info exists seen($f)]} {
    set seen($f) $fn
    set fd [open $files($f)]
    set re {#include\s*[<"]([^"]+)[>"]} ;# "]"]"
    while {[gets $fd line] >= 0} {
      if {[regexp $re $line - path] && [expand $path]} {
        continue
      }
      puts $line
    }
    close $fd
  }
  return 1
}

set f [lindex $argv 0]
set ifd [open $f]

foreach x $argv { set omit($x) "" }

set prefix [file dirname $f]
while {[gets $ifd line] >= 0} {
  if {[regexp {%includeDir<(.*)>%} $line - path]} {
    foreach g [glob [file join $prefix $path/*]] {
      set t [file tail $g]
      if {![info exists omit($t)]} { set files($t) $g }
    }
    #parray files
  }
  if {[regexp {%include<(.*)>%} $line - match]} {
    puts $line
    if {$match ne ""} {
      expand $match
    }
  } else {
    puts $line
  }
}

close $ifd
