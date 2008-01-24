package require ratcl 4
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
    # automatically pick up blocked views (they have a single _B subview)
    if {[view $v names] eq "_B"} {
        set v [view $v blocked]
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
    option -view -configuremethod setView
    delegate option * to table
    delegate method * to table
    method configureTable view {
        set v $view
        #set options($option) $v
        set n [view $v get #]
        if {[$self isViewThick]} {
            # Transpose
            set tagopt row
            set titleopt -titlecols
            $table configure -cols [expr $n + 1] -rows [view $v width] \
                $titleopt 1 \
                -command [mymethod getcell %c %r] \
                -browsecommand [mymethod browsecell %c %r]
        } else {
            # Normal display
            set tagopt col
            set titleopt -titlerows
            $table configure -rows [expr $n + 1] -cols [view $v width] \
                $titleopt 1 \
                -command [mymethod getcell %r %c] \
                -browsecommand [mymethod browsecell %r %c]

            # Set the column widths
            eval $table width [$self getColWidths]
        }

        # Configure a special appearance for subview rows/columns
        $table tag delete subview
        $table tag configure subview -foreground blue
        view $v get @ | tag c | where {($(type) == "V") || ($(type) == "B")} | loop {
            $table tag $tagopt subview $(c)
        }

        return $v
    }
    method setView {option view} {
        set options($option) $view

        set yscroll {}
        set xscroll {}
        if {[winfo exists $win.t]} {
            set yscroll [$self cget -yscrollcommand] 
            set xscroll [$self cget -xscrollcommand] 
            destroy $win.t
        }
        if {[view $view width] == 0} {
            set n [view $view get #]
            #install table using label $win.t -text "$n rows, no columns"
            # Use a text widget instead of a label since text widgets are scrollable
            # which means whether the table component is a tktable or not, it can
            # be treated the same regardless.
            label $win.l
            set message "$n rows, no columns"
            install table using text $win.t -font [$win.l cget -font] -relief [$win.l cget -relief] -background [$win.l cget -background] -height 1 -width [string length $message] -wrap none -yscrollcommand $yscroll -xscrollcommand $xscroll
            destroy $win.l
            $win.t insert end $message 
            $win.t configure -state disabled
        } else {
            install table using table $win.t -rowstretchmode none -colstretchmode all -yscrollcommand $yscroll -xscrollcommand $xscroll
            $self configureTable $view
            bind $table <FocusIn> {
                if {[llength [%W tag cell active]] == 0} {
                    %W activate [%W cget -titlerows],[%W cget -titlecols]
                }
            }
        }
        pack $table -expand 1 -fill both
    }
    constructor {view args} {
        package require Tktable
        $self setView -view $view
        $self configurelist $args
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
    method activate {r c} {
        $table activate [expr $r + [$table cget -titlerows]],[expr $c + [$table cget -titlecols]]
    }

    method isViewThick {} {
        return [expr {([view $v get #] <= 1) && ([view $v width] > 2)}]
    }

    method cellToStr {r c} {
        switch [lindex [view $v types] $c] {
            V {
                set vs [view $v get $r $c]
                # show row i.s.o. block count for blocked views
                if {[view $vs names] eq "_B"} {
                    set vs [view $vs blocked]
                }
                return "#[view $vs size]"
            }
            B {
                return [string length [view $v get $r $c]]b
            }
            default {
                return [view $v get $r $c]
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
        if {[view $v get #] > 10} {view $v first 10 | to w}
        foreach col [view $v width | tag id | get] {
            lappend widths [view $w do {
                collect {[string length [$self cellToStr $(#) $col]]}
                asview len:I | concat [view len:I def 10]
                max len
                asview len:I | concat [view len:I def 50]
                min len
            }]
        }
        view width def $widths | tag col | colmap {col width} | get
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
        # Don't use the install command because internally snit
        # calls lsearch on $args looking for options.  This lsearch
        # causes an expensive string conversion of the view
        #install table using ratable $win.table $v
        set table [ratable $win.table $v]

        $self configurelist $args

        # Setup scrolling 
        $win.table configure -xscrollcommand [mymethod sbset $win.sx] \
            -yscrollcommand [mymethod sbset $win.sy]
        scrollbar $win.sy -orient vertical -command [list $win.table yview]
        scrollbar $win.sx -orient horizontal -command [list $win.table xview]
        grid $win.sy -row 0 -column 2 -sticky ns
        grid $win.sx -row 1 -column 1 -sticky ew
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
    variable topTable
    variable viewvar
    component pw
    option -viewvariable -configuremethod setViewVariable
    option -view -configuremethod setTopView
    option -tables -readonly 1
    constructor {v args} {
        set wid 0
        install pw using panedwindow $win.pw; pack $pw -expand 1 -fill both
        set topTable [$self ratable $v]
        $pw add $topTable -sticky nw
        $self configurelist $args
    }
    method setTopView {option view} {
        $topTable configure -view $view
        $topTable configure -browsecommand [mymethod browsecell $topTable %r %c]
        set options(-view) $view
    }
    method viewVarWriteTrace {name1 name2 op} {
        switch -- $op {
            write {
                $self deselect $topTable
                #$self setTopView -view @[myvar viewvar]
                $self setTopView -view $viewvar
            }
        }
    }
    method setViewVariable {option varname} {
        upvar #0 $varname [myvar viewvar]   ;# Link the given global scoped variable to an instance var
        set options($option) $varname
        if {[info exists viewvar]} {
            #$self setTopView -view @[myvar viewvar]
            $self setTopView -view $viewvar
        }

        # The remove ensures there will only be a single trace even with multiple
        # calls to configure different view variables.  The remove silently
        # ignores attempts to remove non-existent traces.
        trace remove variable viewvar write [mymethod viewVarWriteTrace]
        trace add variable viewvar write [mymethod viewVarWriteTrace]
    }
    destructor {
        if {[string length [trace info variable viewvar]] > 0} {
            trace remove variable viewvar write [mymethod viewVarWriteTrace]
        }
    }
    method deselect t {
        # Remove the previous selection tag
        catch {$t selection clear 0,0 end}

        # Delete all panes to the right
        set idx [lsearch -exact [$pw panes] $t]
        incr idx
        if {$idx < [llength [$pw panes]]} {
            eval [linsert [lrange [$pw panes] $idx end] 0 destroy]
            set options(-tables) [lrange $options(-tables) 0 [expr {$idx - 1}]]
        }
    }
    method ratable v {
        incr wid; set t $pw.t$wid
        scrolledratable $t $v -browsecommand [mymethod browsecell $t %r %c]
        bind $t.table.t <Control-r> [mymethod browserow $t]
        bind $t.table.t <Control-t> [mymethod newtoplevel $t]
        lappend options(-tables) $t
        return $t
    }
    method newtoplevel t {
        # Display the current table in its own toplevel browser 
        view [$t cget -view] ratk
    }
    method browsecell {t r c} {
        # Move the selection
        $self deselect $t
        $t selection set [$t tag cell active]
        $t tag raise sel

        # Create the new table and add a pane
        set v [$t cget -view]
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
    method browserow t {
        set r [$t index active row]
        set v [$t cget -view]

        # Move the selection
        $self deselect $t
        $t selection set $r,0 $r,[view $v width]

        # View a single row (ratable automatically transposes this case)
        incr r -[$t cget -titlerows]
        view $v remap $r | to v
        incr wid; set t $pw.t$wid
        set svw [scrolledratable $t $v -browsecommand [mymethod browsecell $t %r %c]]
        after idle $pw add $svw -sticky nw
    }
}
