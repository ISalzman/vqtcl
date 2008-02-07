ratcl::vopdef extend {v colname expr} {
  view $v pair [view $v collect $expr | asview $colname]
}

# Converts the columns identified by the given column names into string type if not already
ratcl::vopdef tostring {v args} {
    set cols [view name vdef $args]
    view $v get @ | join_i $cols | where {$(type) != "S"} | loop {
        view $v mapcols -omit $(name) | pair [view $v get * $(name) | asview $(name)] | to v
    }
    return $v
}

# TODO: Would be nice to accept a list of columns as input for r.  This would correspond to
# "Some Fix Columns" in the examples at http://search.cpan.org/~bdulfer/Data-Pivot-0.05/Pivot.pm
# TODO: Would be nice to have nested pivot tables like I've see some examples of.  I don't 
# understand yet how to implement that.  See the first picture at 
# http://www.microsoft.com/dynamics/using/excel_pivot_tables_collins.mspx for an example of
# what I'm talking about.  I think "multiple levels" is a better term than "nested"
#
# v - the input view
# r - the name of the column whose values will become the row labels in the leftmost cols
# c - the name of the column whose values will become the column labels in the other columns
# A totals column and totals row will automatically be added.
# op - a ratcl vop that can operate on a view (i.e. "size", "sum somesubcol", etc.)
#      any columns that have been "grouped away" can be operated on.  The empty
#      string can be passed in in order to preserve the subviews unchanged.  This is
#      useful for using with tkratcl viewer where the subviews can be expanded and collapsed
# optype - the type of value returned by op
ratcl::vopdef pivot {v r c {op size} {optype I}} {
    # Since "Total" is a string, both the row and column must be type string.
    view $v tostring $r $c | to v

    # Ensure all combinations of rows and columns exist
    view $v project $c | to cols
    view $v project $r | to rows
    view $rows product $cols | join $v g | to rcg

    # Row, column and grand totals
    view $v mapcols -omit $c | group $r g | extend $c {"Total"} | \
        mapcols $r $c g | to rtotals
    view $v mapcols -omit $r | group $c g | extend $r {"Total"} | \
        mapcols $r $c g | to ctotals
    view $v mapcols -omit $r $c | extend $r {"Total"} | \
        extend $c {"Total"} | group $r $c g | to total
    view $rcg concat $rtotals | concat $ctotals | concat $total | to rcg

    view $rcg do {
        # Perform the group operation on the grouped column
        extend _d:$optype "\[view \$(g) $op\]"
        mapcols -omit g

        # Group the rows together by column
        group $c g | to v

        # First column of the output is the row titles
        get 0 g | mapcols $r | to out
    }

    # Fill the remaining columns with each $c
    view $v loop {
        set all [view $(g) rename _d $($c) | mapcols -omit $r]
        set out [view $out | pair $all]
    }
    return $out
}

