#!/usr/bin/env tclkit
# xml2mk -=- convert an XML file to a Metakit database

# This code has several drawbacks:
#  - it's slow and needs a lot of working memory (25..50x input)
#     (can be fixed with Vlerq's new XML scanner/tokenizer, see "-n" below)
#  - missing attr's cannot be distinguished from attr's with an empty value
#     (largely due to the lack of NULL support in the MK v2 file format)
#  - see http://www.vlerq.org/vqd/298 for background info on this MK mapping
#  - does not properly deal with <!-- ... --> comments: remove them first
#
# Based heavily on Stephen Uhler's HTML parser in 10 lines
# Modified by Eric Kemp-Benedict for XML
# Mostly rewritten and tied to Mk4tcl by Jean-Claude Wippler
# see also http://wiki.tcl.tk/tax
#
# Namespace "tax" stands for "Tiny API for XML"

package require Mk4tcl

namespace eval tax {
  namespace eval v {
    variable stk ;# current stack of tags
    variable row ;# current stack of parent row #'s
    variable def ;# list of all tags encountered so far
    variable nod ;# array: "tag" -> list of attrs
		  #	   "tag,attr" -> index if seen
		  #	   "tag,row" -> list of child ref pairs
  }
  proc parse {x} {
    set v::stk {}
    set v::row {}
    set v::def {}
    rparse $x
  }
  proc rparse {x} {
    set x [string map {\{ &ob; \} &cb;} $x]
    set e {<(/?)([^\s/>]+)\s*([^/>]*)(/?)>}
    append s \} {; node {\1} {\2} {\3} {\4} } \{
    eval "node {} xml {} {} {[regsub -all $e $x $s]}; node / xml {} {} {}"
  }
  proc node {cl1 tag attrs cl2 bdy} {
    set tag [string map {: _ - _} $tag]
    # adjust XML header information, it interferes with attribute parsing
    if {[string index $tag 0] eq "?"} {
      set tag [string range $tag 1 end]
      set bdy [string range $attrs 0 end-1]
      set attrs ""
    }
    set n [lsearch -exact $v::def $tag]
    if {$n < 0} {
      set n [addrow _N [list n $tag]]
      #puts "n $n tag $tag"
      lappend v::def $tag
      set v::nod($tag) _
      mk::view layout db.$tag _:I
    }
    set pn [lindex $v::stk end]
    set ps [lsearch -exact $v::def $pn]
    set pr [lindex $v::row end]
    if {$pr eq ""} { set pr -1 }
    if {$cl2 ne ""} {
      set type "OC"
    } elseif {$cl1 ne ""} {
      set v::stk [lrange $v::stk 0 end-1]
      set v::row [lrange $v::row 0 end-1]
      return
    } else {
      if {[string index $tag 0] ne "?"} {
	lappend v::stk $tag
	lappend v::row 0
      }
      set type "O"
    }
    set props {}
    set attrs [regsub -all {\s+|(\s*=\s*)} $attrs " "]
    if {[catch { llength $attrs }]} { puts "attrs?"; set attrs "" }
    foreach {x y} $attrs {
      set x [string map {: _} $x]
      if {![info exists v::nod($tag,$x)]} {
	set v::nod($tag,$x) [llength $v::nod($tag)]
	lappend v::nod($tag) $x
      }
      lappend props $x $y
    }
    set m [addrow $tag $props]
    if {$pr >= 0} {
      lappend v::nod($pn,$pr) $n $m
      if {$::new} { addrow _N!$n.t "" }
    }
    set v::nod($tag,$m) {}
    if {$type eq "O"} {
      set bdy [string trim $bdy]
      if {$bdy ne ""} {
	set t [addrow _N!$n.t [list s $bdy]]
	if {!$::new} { lappend v::nod($tag,$m) 0 $t }
      }
    }
  }
  proc addrow {tag vals} {
    set c [eval [linsert $vals 0 mk::row append db.$tag]]
    mk::cursor pos c
  }
}

# The following variant uses Vlerq's tokenizer, activate with "-n" option
#
# Input file size 7.9 Mb, output file size 7.1 Mb - itml.xml
# Less memory, but also much slower (inefficient thrive callback probably):
#   regexp   152 Mb mem   170 sec
#   xmltok   104 Mb mem   470 sec
# Much of remaining memory use is probably MK: no interim commits are done.

namespace eval tax {
  namespace eval v {
    variable cur ;# array with current token state
  }
  proc vparse {x} {
    node {} xml {} {} {}
    set v::cur(e) ""
    set v::cur(p) _
    set v::cur(n) 0
    thrill::vcall { xparser Wn } S $x
    while {1} {
      foreach {a b} [::thrill::vcall xmltokens] {
	xtoken $a $b
	if {$a eq ""} break
      }
      if {$a eq ""} break
    }
    xtoken < ""
    node / xml {} {} {}
  }
  proc xtoken {a b} {
    switch -- $a {
      < { if {$v::cur(e) ne ""} {
	    node $v::cur(o) $v::cur(e) $v::cur(a) $v::cur(c) $v::cur(b)
	  }
	  set a o
	  array set v::cur {e "" a "" o "" c "" b ""}
	}
      / { if {$v::cur(p) eq "o" } {
	    set a o
	    set v::cur(o) /
	  } else {
	    set v::cur(c) /
	  }
	}
      I { if {$v::cur(p) eq "o"} {
	      set v::cur(e) $b 
	    } else {
	      lappend v::cur(a) $b ""
	    }
	}
      Q { lset v::cur(a) end [string range $b 1 end-1] }
      T { append v::cur(b) $b }
      D { append v::cur(b) $b }
    }
    set v::cur(p) $a
    incr v::cur(n)
  }
}

# the -n option switches to the new XML scanner in Thrive, instead of the
# above Tcl-based version - note that the Tcl version:
#   - needs more mem
#   - is faster for now
#   - does not properly deal with comments
# But the Thrive version needs the Thrive VM, i.e. the "./ratcl.so" file.

set new 0
if {[lindex $argv 0] eq "-n"} {
  set new 1
  set argv [lrange $argv 1 end]
  load ./ratcl[info sharedlibext]
  package require ratcl
  namespace import ratcl::*
  rename tax::rparse ""
  rename tax::vparse tax::rparse
}

if {[llength $argv] != 2} {
  puts stderr "Usage: $argv0 ?-n? inxml outmk"
  exit 1
}

foreach {ifn ofn} $argv break

set fd [open $ifn]
fconfigure $fd -encoding utf-8

file delete $ofn
mk::file open db $ofn
mk::view layout db._N {n:B {r {c:I d:I}} {t s:B}}

puts Reading
set data [read $fd]
close $fd

puts Parsing
tax::parse $data

#parray tax::v::nod

puts Adjusting
foreach x $tax::v::def {
  puts -nonewline [format {  %-30s } $x]; flush stdout
  mk::view layout db.$x [string replace [join $tax::v::nod($x) :B,]:B 2 2 I]
  set n [mk::select db._N n $x]
  set refs db._N!$n.r
  mk::loop c db.$x {
    mk::set $c _ [mk::view size $refs]
    set i [mk::cursor position c]
    foreach {a b} $tax::v::nod($x,$i) {
      mk::row append $refs c $a d $b
    }
  }
  set k [mk::view size db.$x]
  set c [mk::view size $refs]
  set t [mk::view size db._N!$n.t]
  if {$new} {incr t -$k} else { incr c -$t }
  puts [format {%6d nodes %7d children %7d texts} $k $c $t]
  mk::row append db.$x _ $c
}

puts Commit
mk::file close db

puts "Done (xml [string length $data]b, db [file size $ofn]b)"
