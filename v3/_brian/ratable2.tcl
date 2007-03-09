package require ratcl
package require Tktable

# Return the given cell in the view represented by the given tktable window name
proc tableget {t r c} {
    variable viewvar:$t
    set v [set viewvar:$t]
    
    # The first row is the column header
    incr r -1
    if {$r < 0} {
        return [lindex [view $v names] $c]
    } else {
        if {[lindex [view $v types] $c] != "V"} {
            return [vget $v $r $c]
        } else {
            return "#[vget $v $r $c #]"
        }
        }
    }

# Displays subview in new toplevel window
# Displaying the subview as a tktable embedded within the
# outer tktable cell wasn't straightforward to implement.  Tktable
# doesn't automatically expand it's cell size based on contents like
# 'grid' does.
proc displaysubview {t r c} {
    variable viewvar:$t
    set v [set viewvar:$t]
    if {[lindex [view $v types] $c] == "V"} {
        incr r -1
        view $v get $r $c | tkr
        set title "$r - [lindex [view $v names] $c]"
        variable fid
        wm title .$fid $title
    }
}

# Minimum column size of 10, max 50.  Only check the widths
# of the first 10 rows
proc min args {lindex [lsort -increasing -integer $args] 0}
proc max args {lindex [lsort -decreasing -integer $args] 0}
proc getColWidths v {
    set widths {}
    foreach col [view $v get @ | where {$(type) != "V"} | get * name] {
        lappend widths [max 10 [min 50 [eval max \
            [view $v first 10 | collect {[string length $($col)]}]]]]
    }
    view $v get @ | where {$(type) != "V"} | pair [view width vdef $widths] | \
        tag col | mapcols col width | get
}

proc ratable {t v} {
    # Setup a variable to refer to the view.  This way a variable name can be passed to 
    # the callback scripts in order to really ensure no string conversion of the view
    variable viewvar:$t
    set viewvar:$t $v

    # Create the table.  The subview display is handled via the browsecommand.  This is 
    # a bit clumsy (i.e. what if all the columns are subviews, then you can's scroll
    # without expanding every subview), but it was quick to implement.
    table $t -rows [expr [vget $v #] + 1] -cols [view $v width] \
        -titlerows 1 -rowstretchmode none -colstretchmode all \
        -command [namespace code {tableget %W %r %c}] \
        -browsecommand [namespace code {displaysubview %W %r %c}]

    # Set the column widths
    eval $t width [getColWidths $v]

    # Cleanup the variable when the table goes away    
    bind $t <Destroy> +[namespace code [list unset viewvar:$t]]

    # Configure a special appearance for subview columns
    view $v get @ | tag c | where {$(type) == "V"} | loop {
        $t tag col subview $(c)
    }
    $t tag configure subview -foreground blue
    return $t
    }

# Easier to use as a view operator.  Needs a better name.
set fid 0
ratcl::vopdef tkr v {
    variable fid
    incr fid
    toplevel .$fid
    pack [ratable .$fid.1 $v] -expand 1 -fill both
}
