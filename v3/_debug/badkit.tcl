#! /usr/bin/env tclkit
# figure out whu tclkitlite won't run

lappend auto_path .
load ./libvlerq1.dylib
package require vlerq

puts X-[vget {mapf /Users/jcw/tclkits/84/a} @ * 0]
puts Y-[vget {at {mapf /Users/jcw/tclkits/84/a} 0 0} @ * 0]
puts Z-[vget {at {at {mapf /Users/jcw/tclkits/84/a} 0 0} 4 2} @ * 0]

