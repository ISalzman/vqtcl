#!/usr/bin/env tclkit
# docgen -=- generate documentation from a simple Tcl-based markup script

source [file join [file dirname [info script]] xmlgen.tcl]
set xmlgen::VERSION 1.4
package provide xmlgen 1.4

source [file join [file dirname [info script]] htmlgen.tcl]
set htmlgen::VERSION 1.4
namespace import htmlgen::*

namespace eval collect {
  proc info {pairs} {
    array set ::info $pairs
  }
  proc preamble {code} {
    set ::info(preamble) $code
  }
  proc desc {text} {
    set ::desc(~) $text
  }
  proc cmd {name args} {
    lappend ::info(calls) cmd $name
    set ::cmds($name) [lrange $args 0 end-1]
    set ::desc($name) [lindex $args end]
  }
  proc obj {self name args} {
    lappend ::info(calls) obj $name
    set ::this($name) $self
    set ::objs($name) [lrange $args 0 end-1]
    set ::desc($name) [lindex $args end]
  }
  proc section {name desc} {
    lappend ::info(sections) $name
    set ::sect($name) $desc
  }
  proc examples {args} {
    set ::info(examples) $args
  }
}

proc require {name {vers ""}} {
  if {$name eq $::info(package)} { set vers $::info(version) }
  tt - package require [b $name] $vers
  br -
}

proc taglist {pairs} {
  blockquote {
    table cellspacing=0 {
      foreach {x y} $pairs {
	tr {
	  td valign=top nowrap= - [b . $x]
	  td - {&nbsp;}
	  td + $y
	}
      }
    }
  }
}

proc Xtaglist {pairs} {
  blockquote {
    dd compact=compact {
      foreach {x y} $pairs {
	dt - [b $x]
	dd + $y
      }
    }
  }
}

proc do_call {type name} {
  if {$type eq "obj"} {
    set o [var $::this($name)]
    set a $::objs($name)
  } else {
    set a $::cmds($name)
  }
  lappend o [b $name]
  foreach x $a {
    if {[regexp {^\?(.+)} $x - x]} {
      lappend o "?[var . $x]?"
    } elseif {[string match {[a-zA-Z_]*} $x]} {
      lappend o [var . $x]
    } else {
      lappend o $x
    }
  }
  tt - [join $o]
}

proc do_text {text} {
  foreach x [split [regsub -all \n\n $text \x01] \x01] {
    p + $x
  }
}

proc do_sect {name body} {
  h3 - [string toupper $name]
  uplevel 1 [list blockquote ! $body]
}

proc do_html {} {
  global info cmds objs this desc sect
  html {
    head {
      meta http-equiv=Content-type {content=text/html; charset=utf-8} - ""
      title - $info(name) $info(version)
      style type=text/css {+
	<!-- var {color:#44a} pre {background-color:#eef} -->
      }
    }
    body {
      do_sect name {
	p - [b $info(name) - $info(title)]
      }
      if {$info(preamble) ne ""} {
	do_sect synopsis {
	  p ! $info(preamble)
	  foreach {type name} $info(calls) {
	    do_call $type $name
	    br -
	  }
	}
      }
      do_sect description {
	do_text $desc(~)
	dl {
	  foreach {type name} $info(calls) {
	    dt {
	      do_call $type $name
	    }
	    dd {
	      do_text $desc($name)
	    }
	  }
	}
      }
      foreach name $::info(sections) {
	do_sect $name {
	  do_text $sect($name)
	}
      }
      if {$info(examples) ne ""} {
	do_sect examples {
	  foreach {x y} $info(examples) {
	    set x [string trim $x]
	    if {$x ne ""} {
	      do_text $x
	    }
	    regsub {^\n} $y {} y
	    set y [string trimright $y]
	    if {$y ne ""} {
	      pre width=81n - "&nbsp; [esc $y]"
	    }
	  }
	}
      }
      foreach x {author copyright bugs website see-also keywords} {
	if {$info($x) ne ""} {
	  do_sect [string map {- " "} $x] {
	    set y $info($x)
	    switch $x {
	      author    { p - Written by $y. }
	      copyright { p - Copyright {&copy;} $y }
	      keywords  { p - [join $y ", "] }
	      website	{ p - See [a href=$y $y]. }
	      default   { p - $y }
	    }
	  }
	}
      }
    }
  }
}

proc docgen {fn} {
  global info
  foreach x {
    author bugs calls copyright desc examples keywords package preamble
    sections see-also title version website
  } {
    set info($x) ""
  }
  namespace eval collect [list source $fn]
  set info(name) [string totitle $info(package)]
  do_html
}

if {[llength $argv] != 1} { puts stderr "Usage: $argv0 infile"; exit 1 }

docgen [lindex $argv 0]
