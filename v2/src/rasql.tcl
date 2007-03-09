# SQL translation layer for Ratcl

package provide rasql 0.01
package require -exact ratcl 1.0

namespace eval ::thrill {
  namespace eval q {
    variable L -1
    variable C
    variable sgram    ;# sql grammar tables
    variable spars    ;# sql parser object
  }

# the only visible code from ratcl
  proc m_sql {vid stmt} {
    set ast [sqlparse $stmt]
    #puts [list AST $ast]
    do q $vid $ast
  }
# interface to the sql parser
  proc sqlparse {str} {
    if {![info exists q::spars]} {
      if {![info exists q::sgram]} { set q::sgram [readfile ../noarch/sql.db] }
      set q::spars [vcall vparini B(S)II $q::sgram {<= >= <>} 1 1]
    }
    vcall parser VS $q::spars $str
  }
# dispatcher
  proc do {pre vid node} {
    eval [lreplace $node 0 0 ${pre}_[lindex $node 0] $vid]
  }
# set up column info
  proc colinfo {vid idx} {
    set i -1
    foreach c [view $vid info names] t [view $vid info types] {
      lappend q::C($q::L.$c) $idx [incr i]
      lappend q::C($q::L:$idx.$i) $t ""
    }
  }
# sql statement node types
  proc q_sel {vid cl fr wh gb hv ob} {
    vars L C
    incr L
    set C($L) {}
    if {[llength $fr]} {
      set r {}
      foreach {table alias} $fr {
	set v [view $vid get 0 $table]
	set C($L/$table) [llength $C($L)]
	if {$alias ne ""} { set C($L/$alias) [llength $C($L)] }
	lappend C($L) $table
	lappend C($L#) [view $v info width]
	#XXX assumes parent has 1 row
	lappend r $v
      }
      #XXX shortcut to simplify name resolution
      if {[llength $r] == 1} {
	set vid [lindex $r 0]
	colinfo $vid 0
      } else {
	set i -1
	foreach v $r { colinfo $v [incr i] }
      }
    } else {
      set C($L/<none>) 0
      colinfo $vid 0
    }
    #parray C
    if {[llength $wh]} {
      #puts [do g $vid $wh]
      set vid [view $vid where [do t $vid $wh]]
    }
    if {$cl ne "star"} {
      set r {}
      foreach {x y} $cl {
	lappend r [lindex $x 1]
      }
      set vid [eval [linsert $r 0 view $vid project]]
    }
    array unset C $L*
    incr L -1
    return $vid
  }
  proc q_uni {vid a b} {
    view [do q $vid $a] union [do q $vid $b]
  }
  proc q_exc {vid a b} {
    view [do q $vid $a] except [do q $vid $b]
  }
# group types
  proc gbin {vid a b} {
    expr {[do g $vid $a] | [do g $vid $b]}
  }
# collect table reference groups
  interp alias {} ::thrill::g_and {} ::thrill::gbin
  interp alias {} ::thrill::g_or  {} ::thrill::gbin
  interp alias {} ::thrill::g_=   {} ::thrill::gbin
  interp alias {} ::thrill::g_<>  {} ::thrill::gbin
  interp alias {} ::thrill::g_>   {} ::thrill::gbin
  interp alias {} ::thrill::g_>=  {} ::thrill::gbin
  interp alias {} ::thrill::g_<   {} ::thrill::gbin
  interp alias {} ::thrill::g_<=  {} ::thrill::gbin
  interp alias {} ::thrill::g_+   {} ::thrill::gbin
  interp alias {} ::thrill::g_-   {} ::thrill::gbin
  interp alias {} ::thrill::g_*   {} ::thrill::gbin
  interp alias {} ::thrill::g_div {} ::thrill::gbin
  interp alias {} ::thrill::g_-   {} ::thrill::do g
  interp alias {} ::thrill::g_0=  {} ::thrill::do g
  proc g_str {vid a}   { return 0 }
  proc g_int {vid a}   { return 0 }
  proc g_sym {vid a} {
    set x $q::C($q::L.$a)
    if {[llength $x] != 2} { error "$a: ambiguous column reference" }
    expr {1<<[lindex $x 0]}
  }
  proc g_dot {vid a b} {
    set t $q::C($q::L/$a)
    foreach {x y} $q::C($q::L.$b) {
      if {$x == $t} { return [expr {1<<$t}] }
    }
    error "$a.$b: no such column"
  }
# translation to Ratcl expression notation
  proc t_and {vid a b} { return "([do t $vid $a]) && ([do t $vid $b])" }
  proc t_or  {vid a b} { return "([do t $vid $a]) || ([do t $vid $b])" }
  proc t_=   {vid a b} { return "([do t $vid $a]) == ([do t $vid $b])" }
  proc t_<>  {vid a b} { return "([do t $vid $a]) != ([do t $vid $b])" }
  proc t_>   {vid a b} { return "([do t $vid $a]) >  ([do t $vid $b])" }
  proc t_>=  {vid a b} { return "([do t $vid $a]) >= ([do t $vid $b])" }
  proc t_<   {vid a b} { return "([do t $vid $a]) <  ([do t $vid $b])" }
  proc t_<=  {vid a b} { return "([do t $vid $a]) <= ([do t $vid $b])" }
  proc t_+   {vid a b} { return "([do t $vid $a]) +  ([do t $vid $b])" }
  proc t_-   {vid a b} { return "([do t $vid $a]) -  ([do t $vid $b])" }
  proc t_*   {vid a b} { return "([do t $vid $a]) *  ([do t $vid $b])" }
  proc t_div {vid a b} { return "([do t $vid $a]) /  ([do t $vid $b])" }
  proc t_-   {vid a}   { return "- ([do t $vid $a])" }
  proc t_0=  {vid a}   { return "! ([do t $vid $a])" }
  proc t_sym {vid a}   { return $a }
  proc t_str {vid a}   { return "\"[string range $a 1 end-1]\"" }
  proc t_int {vid a}   { return $a }
# attach to variables
  proc vars {args} {
    foreach x $args {
      uplevel 1 [list upvar 0 q::$x $x]
    }
  }
}
