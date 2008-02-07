set fid 0
proc createContainer parent {
    variable fid
    incr fid

    # Create the container
    set frame $parent.ratk$fid
    if {[string length $parent] == 0} {
        set frame [toplevel $frame].t
    }
    return $frame
}
ratcl::vopdef ratk {v {parent ""}} {
    if {[catch {package present Tk}]} {
        package require Tk
        wm withdraw .
    }
    if {[catch {package present Tktable}]} {
        package require Tktable
    }

    set t [createContainer $parent]
    ratclbrowser $t $v
    if {[string length $parent] == 0} {
        pack $t -expand 1 -fill both
    }
    return $t
}

# Written by RS at http://wiki.tcl.tk/3242
proc string2hex {string} {
    set where 0
    set res {}
    while {$where<[string length $string]} {
        set str [string range $string $where [expr $where+7]]
        if {![binary scan $str H* t] || $t==""} break
        regsub -all (....) $t {\1 } t4
        regsub -all (..) $t {\1 } t2
        set asc ""
        foreach i $t2 {
            scan $i %2x c
            append asc [expr {$c>=32 && $c<=127? [format %c $c]: "."}]
        }
        lappend res [format "%7.7x: %-21s %s" $where $t4  $asc]
        incr where 8
    }
    join $res \n
 }

package require snit
snit::widget ratable {
    variable v
    variable titleopt
    component table
    option -browsecommand
    delegate option * to table
    delegate method * to table

    constructor {view args} {
        package require Tktable
        set v $view
        set n [view $v get #]
        if {[view $v width] == 0} {
            install table using label $win.l -text "$n rows, no columns"
        } else {
            if {[$self isViewThick]} {
                # Transpose
                set tagopt row
                set titleopt -titlecols
                install table using table $win.t -cols [expr $n + 1] -rows [view $v width] \
                    $titleopt 1 -rowstretchmode none -colstretchmode all \
                    -command [mymethod getcell %c %r] \
                    -browsecommand [mymethod browsecell %c %r]
                $self configurelist $args
            } else {
                # Normal display
                set tagopt col
                set titleopt -titlerows
                install table using table $win.t -rows [expr $n + 1] -cols [view $v width] \
                    $titleopt 1 -rowstretchmode none -colstretchmode all \
                    -command [mymethod getcell %r %c] \
                    -browsecommand [mymethod browsecell %r %c]
                $self configurelist $args

                # Set the column widths
                eval $table width [$self getColWidths]
            }

            # Configure a special appearance for subview rows/columns
            view $v get @ | tag c | where {($(type) == "V") || ($(type) == "B")} | loop {
                $table tag $tagopt subview $(c)
            }
            $table tag configure subview -foreground blue
            bind $table <FocusIn> {
                if {[llength [%W tag cell active]] == 0} {
                    %W activate [%W cget -titlerows],[%W cget -titlecols]
                }
            }
        }
        pack $table -expand 1 -fill both
    }

    # A -browsecommand wrapper that adjusts for column or row header
    method browsecell {r c} {
        if {[string length $options(-browsecommand)] > 0} {
            # evaluate the browsecommand passing the adjusted row and column
            incr r -[$self cget $titleopt]
            if {$r >= 0} {
                set cmd [string map [list %% % %r $r %c $c] $options(-browsecommand)]
                uplevel #0 $cmd
            }
        }
    }

    method isViewThick {} {
        return [expr {([view $v get #] <= 1) && ([view $v width] > 2)}]
    }

    method cellToStr {r c} {
        switch [lindex [view $v types] $c] {
            V {
                return "#[vget $v $r $c #]"
            }
            B {
                return [string length [vget $v $r $c]]b
            }
            default {
                return [vget $v $r $c]
            }
        }
    }

    method getcell {r c} {
        # The first row is a column or row header
        incr r -[$self cget $titleopt]
        if {$r < 0} {
            return [lindex [view $v names] $c]
        } else {
            return [$self cellToStr $r $c]
        }
    }

    # Minimum column size of 10, max 50.  Only check the widths
    # of the first 10 rows
    method getColWidths {} {
        set widths {}
        set w $v
        if {[vget $v #] > 10} {view $v first 10 | to w}
        foreach col [view $v width | tag id | get] {
            lappend widths [view $w do {
                collect {[string length [$self cellToStr $(#) $col]]}
                asview len:I
                concat [view len:I vdef 10]
                max len
                asview len:I
                concat [view len:I vdef 50]
                min len
            }]
        }
        view width vdef $widths | tag col | mapcols col width | get
    }
}

# Looks like  are two generic snit scrolling options already. By JH:
# http://tcllib.cvs.sourceforge.net/tcllib/tklib/modules/widget/scrollw.tcl?revision=1.11&view=markup
# and scrodgets: http://web.tiscali.it/irrational/tcl/scrodget2.0.1/index.html
# Probably better to use one of those.
snit::widget scrolledratable {
    component table
    delegate option * to table
    delegate method * to table
    constructor {v args} {
        # Create the table
        install table using ratable $win.table $v

        $self configurelist $args

        # Zero column views are displayed using a label widget.  Labels
        # don't have x and y scrollcommand options, so only add the scrollbars
        # if not a label.
        if {[winfo class $win.table] != "Label"} {
            $win.table configure -xscrollcommand [mymethod sbset $win.sx] \
                -yscrollcommand [mymethod sbset $win.sy]
            scrollbar $win.sy -orient vertical -command [list $win.table yview]
            scrollbar $win.sx -orient horizontal -command [list $win.table xview]
            grid $win.sy -row 0 -column 2 -sticky ns
            grid $win.sx -row 1 -column 1 -sticky ew
        }
        grid $win.table -row 0 -column 1 -sticky nsew
    }
    
    # Dynamic scrollbars.  This gem from Joe English: http://wiki.tcl.tk/950
    method sbset {sb first last} {
        if {$first <= 0 && $last >= 1} {
            grid remove $sb
        } else {
            grid $sb
        }
        $sb set $first $last
    }
}
snit::widget ratclbrowser {
    variable wid
    component pw
    constructor {v args} {
        set wid 0
        install pw using panedwindow $win.pw; pack $pw -expand 1 -fill both
        set t [$self ratable $v]
        $pw add $t -sticky nw
    }
    method deselect t {
        # Remove the previous selection tag
        $t selection clear 0,0 end

        # Delete all panes to the right (window leak--should destroy instead?)
        set idx [lsearch -exact [$pw panes] $t]
        incr idx
        if {$idx < [llength [$pw panes]]} {
            eval [linsert [lrange [$pw panes] $idx end] 0 destroy]
        }
    }
    method ratable v {
        incr wid; set t $pw.t$wid
        scrolledratable $t $v -browsecommand [mymethod browsecell $v $t %r %c]
        bind $t.table.t <Control-r> [mymethod browserow $v $t]
        return $t
    }
    method browsecell {v t r c} {
        # Move the selection
        $self deselect $t
        $t selection set [$t tag cell active]
        $t tag raise sel

        # Create the new table and add a pane
        switch [lindex [view $v types] $c] {
            V {
                set svw [$self ratable [view $v get $r $c]]
                after idle $pw add $svw -sticky nw
            }
            B {
                incr wid
                set svw [labelframe $pw.t$wid -text [lindex [view $v names] $c]]
                set text [text $pw.t$wid.t -width 40 -wrap none -font {Courier 9}]; pack $text
                $text insert end [string2hex [view $v get $r $c]] ;# Surprisingly large amount slower.
                #$text insert end [view $v get $r $c]
                after idle $pw add $svw -sticky nw
            }
            default {
                incr wid
                set svw [labelframe $pw.t$wid -text [lindex [view $v names] $c]]
                set text [text $pw.t$wid.t -width 40 -wrap word]; pack $text
                $text insert end [view $v get $r $c]
                after idle $pw add $svw -sticky nw
            }
        }
    }
    method browserow {v t} {
        set r [$t index active row]

        # Move the selection
        $self deselect $t
        $t selection set $r,0 $r,[view $v width]

        # View a single row (ratable automatically transposes this case)
        incr r -[$t cget -titlerows]
        view $v remap $r | to v
        incr wid; set t $pw.t$wid
        set svw [scrolledratable $t $v -browsecommand [mymethod browsecell $v $t %r %c]]
        after idle $pw add $svw -sticky nw
    }
}
