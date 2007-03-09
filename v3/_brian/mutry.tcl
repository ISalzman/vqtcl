#! /usr/bin/env tclkit

lappend auto_path ..
package require ratcl

# evaluate a script, displaying each step as if it was entered interactively
proc show {cmds} {
  set x ""
  set r ""
  foreach c [split $cmds \n] {
    append x $c \n
    set y [string trim $x]
    if {![info complete $x] || $y eq ""} continue
    puts "% $y"
    set r [uplevel $x]
    if {$r ne ""} { puts $r }
    set x ""
  }
  return $r
}

show {
  view {A C D} vdef {1 11 22 2 33 44 3 4 5 3 6 7 3 8 9} | group A B | to orig
  view $orig dump
  view $orig get 2 B | dump
  view orig ref | to view

  view $view get 2 B | set 1 C 66 D 77
  puts $orig; view $orig get 2 B | dump
  view $view get 2 B | set 1 C 666 D 777
  puts $orig; view $orig get 2 B | dump
  view {at {ref orig} 2 1} set 1 C 6666 D 7777
  puts $orig; view $orig get 2 B | dump

  #view $view get 2 B | set 0 C 444
  #puts $orig; view $orig get 2 B | dump
  #view {at {ref orig} 2 1} set 0 C 444
  #puts $orig; view $orig get 2 B | dump

  #view $view get 2 B | set 2 D 999
  #puts $orig; view $orig get 2 B | dump
  #view {at {ref orig} 2 1} set 2 D 999
  #puts $orig; view $orig get 2 B | dump
}
