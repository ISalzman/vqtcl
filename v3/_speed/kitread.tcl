#! /usr/bin/env tclkit
# sample code for http://www.vlerq.org/vqr/271

load ./libvlerq3[info sharedlibext]
lappend auto_path .. .

  package require ratcl
  namespace import ratcl::*

  puts A-[time { set dirs [view /Users/jcw/bin/kitten open | get 0 dirs] }]
  puts B-[time { set flat [view $dirs rename name pname | ungroup files] }]
  puts C-[time { view $flat size }]
  puts D-[view $flat size]
  puts E-[time { set some [view $flat where { $(name) == "pkgIndex.tcl" }] }]
  puts F-[time { view $some size }]
  puts G-[view $some size]

  puts [view $flat dump]
  puts [view $some dump]
  puts [view $flat sort size | last 10 | dump]
