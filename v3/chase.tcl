#!/usr/bin/env tclkit
# chase nasty bugs

source tests/initests.tcl
verbose pe
#testConstraint x 1

package require ratcl

set v [view {A B C} vdef {a b c}]
set w [view $v group A B Y]
set x [view $w group A X]

test 0 {} x {
  view $w struct_mk
} {A:S,B:S,Y[C:S]}

test 1 {} x {
  view $w get 0 Y
} {at {group {vdef {A B C} {a b c}} A B Y} 0 2}

test 2 {} x {
  view $w get 0 Y | struct_mk
} {C:S}

test 3 {} x {
  view $x struct_mk
} {A:S,X[B:S,Y[C:S]]}

test 4 {} x {
  view $x get 0 X
} {at {group {group {vdef {A B C} {a b c}} A B Y} A X} 0 1}

test 5 {} x {
  view $x get 0 X | struct_mk
} {B:S,Y[C:S]}

test 6 {} {
  view $x get 0 X 0 Y
} {at {at {group {group {vdef {A B C} {a b c}} A B Y} A X} 0 1} 0 1}

test 7 {} {
  view $x get 0 X 0 Y | struct_mk
} {C:S}

test 8 {bt 2006-09-14} {
  view {a b c} vdef {1 2 3} group h | save v.mk
} {}
  
::tcltest::cleanupTests
