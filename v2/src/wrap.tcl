#!/usr/bin/env tclkit
# wrap -=- Create a starkit/starpack, like SDX does

#TODO deal with the dependency on vfs::zip for compressing files
# either don't compress, or rely on a zip/gzip command to do the work
#
# gzip -n can produce almost right file: drop first 10 and last 8 bytes, then
# insert 78 + 9C in front and append 4-byte CRC (decompress vs. inflate diff)

if {[llength $argv] != 3} {
  puts stderr "usage: $argv0 srcdir prefixfile outfile"
  exit 1
}
foreach {src pfx out} $argv break

load ./thrive[info sharedlibext]

set base [file dirname [info script]]
source $base/thrill.tcl
source $base/ratcl.tcl
source $base/m2mvfs.tcl

file copy -force $pfx $out

vfs::m2m::Mount $out mem

set nd 0
set nf 0
set dirs .

while {[llength $dirs]} {
  set d [lindex $dirs 0]
  set dirs [lrange $dirs 1 end]
  foreach x [glob -nocomplain -directory $src/$d -tails *] {
    set f [string range $d/$x 2 end]
    if {[file isdir $src/$f]} {
      lappend dirs $d/$x
      file mkdir mem/$f
      incr nd
    } else {
      file copy $src/$f mem/$f
      incr nf
    }
  }
}

vfs::unmount mem

puts "$out: [file size $out] bytes, $nf files, $nd subdirs"
