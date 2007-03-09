#! /usr/bin/env tclkit

load ./libvlerq3[info sharedlibext]
lappend auto_path ..
source ../library/mkclvfs.tcl

#interp alias {} vget {} vlerq::get

vfs::mkcl::Mount ../tests/l.kit-be y
#puts [glob y/*]
#puts [glob y/lib/*]
puts y1-[glob y/lib/cgi1.6/*]
puts y2-[time { glob y/lib/cgi1.6/* } 100]

proc t1 {} {
  set root [view ../tests/l.kit-be mapf]
  set x aaa
  puts sc-[time { set x } 100000]
  puts li-[time { lindex {1 2 3} 1 } 100000]
  set a(A) aa
  puts ar-[time { set a(A) } 100000]
  puts v#-[time { vget $root # } 100000]
  puts v@-[time { vget $root @ } 100000]
  puts v0-[time { vget $root 0 0 } 100000]
  puts vd-[time { vget $root 0 dirs } 100000]
  puts vg-[time { vget $root 0 dirs 0 parent } 100000]
  puts vx-[time { vget $root 0 0 0 1 } 100000]
  set n [vget $root 0 dirs]
  puts vn-[time { vget $n 0 parent } 100000]
  puts vV-[time { view $n get 0 parent } 100000]
  puts v1-[time { vget $n 0 1 } 100000]
}
t1
if 0 {;# biggie, PowerBook 1.0 GHz, PPC 10.4.6
  y1-y/lib/cgi1.6/cgi.tcl y/lib/cgi1.6/pkgIndex.tcl
  y2-1738.37 microseconds per iteration
  sc-0.6888 microseconds per iteration
  li-0.76053 microseconds per iteration
  ar-0.82638 microseconds per iteration
  v#-1.75075 microseconds per iteration
  v@-1.74945 microseconds per iteration
  v0-1.97938 microseconds per iteration
  vd-1.94921 microseconds per iteration
  vg-2.49815 microseconds per iteration
  vx-2.38263 microseconds per iteration
  vn-1.84058 microseconds per iteration
  vV-3.51658 microseconds per iteration
  v1-1.78965 microseconds per iteration
  k1-k/lib/cgi1.6/cgi.tcl k/lib/cgi1.6/pkgIndex.tcl
  k2-2086.21 microseconds per iteration
  mg-3.88408 microseconds per iteration
}
if 0 {;# teevie, AMD 3400+, x86_64 Linux 2.6.16
  y1-y/lib/cgi1.6/pkgIndex.tcl y/lib/cgi1.6/cgi.tcl
  y2-76.19 microseconds per iteration
  li-0.23827 microseconds per iteration
  ar-0.24595 microseconds per iteration
  v#-0.47444 microseconds per iteration
  v@-0.46889 microseconds per iteration
  v0-0.59416 microseconds per iteration
  vd-0.58205 microseconds per iteration
  vg-0.75461 microseconds per iteration
  vx-0.7216 microseconds per iteration
  vn-0.54248 microseconds per iteration
  v1-0.50789 microseconds per iteration
  k1-k/lib/cgi1.6/cgi.tcl k/lib/cgi1.6/pkgIndex.tcl
  k2-134.7 microseconds per iteration
  mg-1.24412 microseconds per iteration
}

vfs::mk4::Mount ../tests/l.kit-be k
#puts [glob k/*]
#puts [glob k/lib/*]
puts k1-[glob k/lib/cgi1.6/*]
puts k2-[time { glob k/lib/cgi1.6/* } 100]

puts mg-[time { mk::get exe.dirs!0 parent } 100000]
