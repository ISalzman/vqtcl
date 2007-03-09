#!/usr/bin/env tclkit
# stripth -=- Strip comments from Thrill scripts

# usage: stripth ?from-sect? ?to-sect? <infile >outfile

set first [lindex $argv 0]
set last [lindex $argv 1]
if {$last eq ""} { set last $first }
set skip [expr {$first ne ""}]
set prev ?

while {[gets stdin line] != -1} {
  if {[regexp {^# <(\w+)> ---} $line - sect]} {
    if {$prev eq $last} break
    if {$sect eq $first} { set skip 0 }
    set prev $sect
  }
  if {$skip} continue
  if {$line eq ""} continue
  if {[string match {[\\#]*} $line]} continue
  if {[string match {D *} $line]} continue
  set line [regsub -all { #[-\w*':]+( |$)} $line { }]
  set line [regsub -all {[ \t]+} $line { }]
  puts $line
}
