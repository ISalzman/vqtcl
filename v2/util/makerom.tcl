#!/usr/bin/env tclkit
# makerom -=- Make a "ROM" for loading in Tclkit Lite

set uncomp 0
if {[lindex $argv 0] eq "-u"} {
  set uncomp 1
  set argv [lrange $argv 1 end]
}

if {[llength $argv] != 2} {
  puts stderr "Usage: $argv0 ?-u? romin romout"
  exit 1
}

set fd [open [lindex $argv 0]]
fconfigure $fd -translation binary
set rom [read $fd]
close $fd

set xo [lindex $argv 1]

set u [string length $rom]
if {$uncomp} {
  set comp $rom
} else {
  set comp [vfs::zip -mode compress $rom]
}
set z [string length $comp]

set fd [open $xo w]
fconfigure $fd -translation binary
puts -nonewline $fd $comp
puts -nonewline $fd tXiS
puts -nonewline $fd [binary format II $u $z]
close $fd

puts " $xo: [string length $rom] -> [file size $xo] bytes"
