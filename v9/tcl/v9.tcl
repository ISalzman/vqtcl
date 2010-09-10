# V9 Tcl script code
# 2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
# $Id$

package require Tcl 8.5
package provide V9 $v9(version)

set v9(typemap) { N 0  I 1  L 2  F 3  D 4  S 5  B 6  V 7 }
set v9(typerev) { 0 N  1 I  2 L  3 F  4 D  5 S  6 B  7 V }

namespace eval v9 {
    namespace export -clear {[a-z]*}
    namespace ensemble create

# first the bare essentials needed to deal with views and meta-view structures,
# the "meta", "size", and "at" core primitives can be replaced by a C version
    
    # adapted from readkit.tcl, converts Metakit description to a Mk4tcl layout
    proc Meta2list {m} {
        set m [regsub -all {(\w+)\s*\[} $m "\{\\1 \{"]
        return [string map {] "\}\}" , " "} $m]
    }

    # reconstruct the columns view from a MK-style description string
    #TODO Meta2cols might be a good candidate for memoization
    proc Meta2cols {m} {
        lappend o vdat name:S,type:I,subv:V
        set names {}
        set types {}
        set subvs {}
        foreach x [Meta2list $m] {
            if {[llength $x] == 1} {
                lassign [split ${x}:S :] n t
                lappend names $n
                lappend types $t
                lappend subvs ""
            } else {
                lassign $x n s
                lappend names $n
                lappend types V
                lappend subvs [join $s ,] ;#FIXME wrong for nested subviews
            }
        }
        lappend o $names [string map $::v9(typemap) $types] $subvs
        return $o
    }
    
    # core primitive: return the number of rows in view v
    proc size {v} {
        if {[llength $v] <= 1} {
            if {[string is integer -strict $v]} {
                return $v
            }
            return [llength [Meta2list $v]]
        }
        return [llength [lindex $v 2]]
    }

    # core primitive: return the meta-view of view v
    proc meta {v} {
        if {[llength $v] <= 1} {
            if {[string is integer -strict $v]} {
                return ""
            }
            return {name:S,type:I,subv:V}
        }
        return [lindex $v 1]
    }
    
    # core primitive: return the element in view v at row r and column c
    proc at {v r c} {
        if {[llength $v] <= 1} {
            # meta-views are a special case
            set v [Meta2cols [lindex $v 0]]
        }
        return [lindex $v [incr c 2] $r]
    }
}

# the following definitions are based on the above bare essentials and should
# work with either the above Tcl implementation or their C replacements in V9x
    
namespace eval v9 {

    proc OneCol {v col} {
      set n [size $v]
      set o {}
      for {set r 0} {$r < $n} {incr r} {
        lappend o [at $v $r $col]
      }
      return $o
    }

    # return the list of a column names of view v
    proc names {v} {
        # return [lindex [Meta2cols [meta $v]] 2]
        return [OneCol [meta $v] 0]
    }

    # return the list of a column types of view v
    proc types {v} {
        # return [string map $::v9(typerev) [lindex [Meta2cols [meta $v]] 3]]
        return [string map $::v9(typerev) [OneCol [meta $v] 1]]
    }

    # return the number of columns in view v
    proc width {v} {
        return [size [meta $v]]
    }

    # loop to iterate over items in a view and collect the results
    proc loop {v ivar vars body} {
      upvar 1 $ivar i
      foreach x $vars {
        upvar 1 $x _$x
      }
      set o {}
      set n [size $v]
      for {set i 0} {$i <$n} {incr i} {
        set c -1
        foreach x $vars {
          set _$x [at $v $i [incr c]]
        }
        set e [catch [list uplevel 1 $body] r]
        switch $e {
          0 - 2 {}
          1     { return -code $e $r }
          3     { return $o }
          4     { continue }
        }
        lappend o $r
      }
      return $o
    }

    proc OneRow {v row {named ""}} {
      if {$named ne ""} {
        set m [names $v]
      }
      set n [width $v]
      set o {}
      for {set c 0} {$c < $n} {incr c} {
        if {$named ne ""} {
          lappend o [lindex $m $c]
        }
        lappend o [at $v $row $c]
      }
      return $o
    }

    # lookup column names
    proc ColNum {v argv} {
      set names [names $v]
      set o {}
      foreach x $argv {
        if {![string is integer -strict $x]} {
          set y [lsearch -exact $names $x]
          if {$y >= 0} {
            set x $y
          }
        }
        lappend o $x
      }
      return $o
    }

    # general access operator
    proc get {v args} {
      if {[llength $args] == 0} {
        lappend args * *
      }
      while {[llength $args] > 0} {
        set args [lassign $args row col]
        switch $row {
          \# {
            return [size $v]
          }
          @ {
            set v [meta $v ]
            if {$col eq ""} { return $v }
            set args [linsert $args 0 $col]
            continue
          }
          * {
            switch $col {
              * { return [concat {*}[loop $v i {} { OneRow $v $i }]] }
              "" { return [loop $v i {} { OneRow $v $i }] }
              default { return [OneCol $v [ColNum $v $col]] }
            }
          }
          default {
            switch $col {
              * { return [OneRow $v $row] }
              "" { return [OneRow $v $row -named] }
              default { set v [at $v $row [ColNum $v $col]] }
            }
          }
        }
      }
      return $v
    }

    # return a pretty text representation of a view, without nesting
    proc dump {v {maxrows 20}} {
      set n [size $v]
      if {[width $v] == 0} { return "  ($n rows, no columns)" }
      set i -1
      foreach x [names $v] y [types $v] {
        if {$x eq ""} { set x ? }
        set v2 [get $v * [incr i]]
        switch $y {
          B       { set s { "[string length $r]b" } }
          V       { set s { "#[size $r]" } }
          default { set s { } }
        }
        set c {}
        foreach r $v2 {
          lappend c [eval set r $s]
        }
        set c [lrange $c 0 $maxrows-1]
        set w [string length $x]
        foreach z $c {
          if {[string length $z] > $w} { set w [string length $z] }
        }
        if {$w > 50} { set w 50 }
        switch $y {
          B - I - L - F - D - V   { append fmt "  " %${w}s }
          default                 { append fmt "  " %-$w.${w}s }
        }
        append hdr "  " [format %-${w}s $x]
        append bar "  " [string repeat - $w]
        lappend d $c
      }
      set r [list $hdr $bar]
      for {set i 0} {$i < $n} {incr i} {
        if {$i >= $maxrows} break
        set cmd [list format $fmt]
        foreach x $d { lappend cmd [regsub -all {[^ -~]} [lindex $x $i] .] }
        lappend r [eval $cmd]
      }
      if {$i < $n} { lappend r [string map {- .} $bar] }
      ::join $r \n
    }

    # return a pretty html representation of a view, including nesting
    proc html {v {styled 1}} {
      set names [names $v]
      set types [types $v]
      set o <table>
      if {$styled} {
        append o {<style type="text/css"><!--\
          table { font: 8.5pt Verdana; }\
          tt table, pre table { border-spacing: 0; border: 1px solid #aaaaaa; }\
          th { background-color: #eeeeee; font-weight: normal; }\
          td { vertical-align: top; }\
          th, td { padding: 0 4px 2px 0; }\
          th.row,td.row { color: #cccccc; font-size: 80%; }\
        --></style>}
      }
      append o \n {<tr><th class="row"></th>}
      foreach x $names {
        if {$x eq ""} { set x ? }
        append o <th><i> $x </i></th>
      }
      append o </tr>\n
      set n [size $v]
      for {set r 0} {$r < $n} {incr r} {
          append o {<tr><td align="right" class="row">} $r </td>
          set i -1
          foreach x $names y $types val [get $v $r *] {
              switch $y {
                  b - B   { set z [string length $val]b }
                  v - V   { set z \n[html $val 0]\n }
                  default { 
                      set z [string map {& &amp\; < &lt\; > &gt\;} $val] 
                  }
              }
              switch $y {
                  s - S - v - V { append o {<td>} }
                  default { append o {<td align="right">} }
              }
              append o $z </td>
          }
          append o </tr>\n
      }
      append o </table>
    }

    # define a new view from one long list of arguments
    proc def {m args} {
        lappend o vdat $m
        set cols [size $m]
        for {set c 0} {$c < $cols} {incr c} {
            set d {}
            for {set r $c} {$r < [llength $args]} {incr r $cols} {
                lappend d [lindex $args $r]
            }
            lappend o $d
        }
        return $o
    }

    # define a view with a single vector of integers, often used as a map
    proc ivec {args} {
        return [def :I {*}$args]
    }
}
