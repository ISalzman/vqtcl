ratcl::vopdef extend {v colname expr} {
  view $v pair [uplevel 1 [list view $v collect $expr | asview $colname]]
}
ratcl::vopdef vsplit {v col subcol} {
    view $v extend [list ${col}.split $subcol] "\[view [list $subcol] def \$($col)]"
}

# Nested group.  Args is an alternating list of column names and subview column
# name. I.e.  'view $v ngroup {a b} g c g d g' is shorthand for 
# view $v group {a b c d} g | group {a b c} g | group {a b} g
# In other words, group by {a b}, then by c, then by d
ratcl::vopdef ngroup {v args} {
   view {col subname} do {
       def $args 
       to cols
       extend cols {[join [view $cols first [expr [view $cols size] - $(#)] | colmap col | get] { }]}
       extend cmd {"group"}
       colmap {cmd cols subname}
       get * | to groupcmd
       use $v | do [join $groupcmd \n]
   }
}
