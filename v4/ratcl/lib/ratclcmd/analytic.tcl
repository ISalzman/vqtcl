# Add newcol column containing the value of col from
# the previous row.  The size of the resulting view is
# one smaller than the original view as there is no
# previous row for the first row.
ratcl::vopdef lag {v col newcol} {
    view $v last [expr [view $v size] - 1] | to l
    view $v do {
        first [expr [view $v size] - 1]
        colmap $col
        rename [list $col $newcol] 
        to f
    }
    view $l pair $f
}

# Add newcol column containing the value of col from
# the next row.  The size of the resulting view is
# one smaller than the original view as there is no
# next row for the last row.
ratcl::vopdef lead {v col newcol} {
    view $v first [expr [view $v size] - 1] | to f
    view $v do {
        last [expr [view $v size] - 1]
        colmap $col
        rename [list $col $newcol] 
        to l
    }
    view $f pair $l
}

# Group by the given cols.  Transform the resulting
# subview column using body.  Ungroup by the transformed
# subview column
ratcl::vopdef partition {v cols body} {
    # Group
    view $v group $cols _g1 | to v1

    # Apply body to each subview
    view $v1 collect {[view $(_g1) do $body]} | to g2

    # Pair and ungroup
    # Ideally, ungroup would work on 'asview _g2:V'
    # Since it doesn't, be explicit: 'asview {_g2 {col1 col2 ...}}'
    view [lindex $g2 0] names | to subcols
    view $v1 do {
        colomit _g1
        pair [view $g2 asview [list _g2 $subcols]]
        ungroup _g2
    }
}

