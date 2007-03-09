#! /usr/bin/env tclkit
#source ratcl.vfs/main.tcl
lappend auto_path ..

package require Tk
package require ratcl
catch {namespace import ratcl::*}
# Create a view command from the given view
# Needed for callbacks, where shimmering can cause a normal view ref to be lost
variable vid 0
proc getViewCmd v {
    variable vid
    set vcmd view-$vid
    view $v as $vcmd
    incr vid
    return $vcmd
}
# frame, view command, row, column
proc expandSubview {f v r c} {
    $f.b configure -command [list collapseSubview $f $v $r $c]
    pack [ratable $f.t [$v get $r $c]]
}
proc collapseSubview {f v r c} {
    $f.b configure -command [list expandSubview $f $v $r $c]
    destroy $f.t
}
# Creates tablular widget containing the given view's data
proc ratable {win v {maxRows 20}} {
    frame $win -bg black
    set vcmd [getViewCmd $v]
    bind $win <Destroy> "+rename $vcmd {}"
    
    # Display the header row
    set header {}
    foreach col [view $v names] {
        lappend header [label $win.header-$col -text $col]
    }
    eval grid $header -sticky news -padx 1 -pady 1
    
    # Use a text widget's background color for the body
    text $win.temptext; set bg [$win.temptext cget -background]; destroy $win.temptext
    
    # Display the body rows
    set id 0
    view $v first $maxRows | loop c {
        set row {}
        foreach col [view $v names] coltype [view $v types] {
            if {$coltype == "V"} {
                # Subviews are an expandable button
                lappend row [frame $win.$id -bg $bg]
                pack [button $win.$id.b -bg $bg -text $c($col) -command [list expandSubview $win.$id $vcmd $c(#) $col]]
            } else {
                # Labels for everthing else
                lappend row [label $win.$id -bg $bg -text $c($col)]
            }
            incr id
        }
        eval grid $row -sticky news -padx 1 -pady 1
    }
    
    if {[vget $v #] > $maxRows} {
        # Indicate more rows are undisplayed
        set row {}
        foreach col [view $v names] {
            lappend row [label $win.$id -bg $bg -text ...]
            incr id
        }
    }
    eval grid $row -sticky news -padx 1 -pady 1

    return $win
}
# Pass in the view command and var names of any contributing views
variable showid 0
proc tkshow {tw cmd args} {
    set result [uplevel view $cmd]
    variable showid
    incr showid
    set top $tw.$showid
    frame $top -bg [$tw cget -background]
    
    # Create widgets for contributing views
    set id 0
    foreach v $args {
        lappend header [label $top.vn$id -text $v]
        lappend body [ratable $top.vv$id [uplevel view $v]]
        incr id
    }
    
    # Create widgets for results
    lappend header [label $top.cmd -text $cmd -wraplength 200]
    lappend body [ratable $top.res $result]
    
    # Header in one row and body in other row
    eval grid $header
    eval grid $body -padx 10 -sticky n -pady 10
    
    # Insert into text widget
    $tw window create end -window $top
    $tw insert end \n
    return $result
}
proc createDisplayWidget top {
    set f [frame $top.f]
    pack $f -expand 1 -fill both
    set tw $f.t; set sy $f.sy
    set sx $top.sx
    text $tw -yscrollcommand [list $sy set] -xscrollcommand [list $sx set] -width 110
    scrollbar $sy -orient v -takefocus 0 -command [list $tw yview]
    pack $tw -side left -expand 1 -fill both
    pack $sy -expand 1 -fill y
    pack [scrollbar $sx -takefocus 0 -orient h -command [list $tw xview]] -fill x
    return $tw
}
proc gdemo tw {
    set frequents [view {drinker beer perweek:I} vdef {
      adam lolas 1
      woody cheers 5
      sam cheers 5
      norm cheers 3
      wilt joes 2
      norm joes 1
      lola lolas 6
      norm lolas 2
      woody lolas 1
      pierre frankies 0
    }]
    
    set likes [view {drinker beer perday:I} vdef {
      adam bud 2
      wilt rollingrock 1
      sam bud 2
      norm rollingrock 3
      norm bud 2
      nan sierranevada 1
      woody pabst 2
      lola mickies 5
    }]
    
    set serves [view {bar beer quantity:I} vdef {
      cheers bud 500
      cheers samaddams 255
      joes bud 217
      joes samaddams 13
      joes mickies 2222
      lolas mickies 1515
      lolas pabst 333
      winkos rollingrock 432
      frankies snafu 5
    }]
    tkshow $tw {$frequents where { $(perweek) > 4 }} \$frequents
    tkshow $tw {$frequents group drinker drinks} \$frequents
    tkshow $tw {$frequents group drinker G | get 2 G} \$frequents
    tkshow $tw {$likes join $serves J} \$likes \$serves
    tkshow $tw {$likes join $serves J | get 0 J} \$likes \$serves
    tkshow $tw {$likes join $serves J | ungroup J} \$likes \$serves
    tkshow $tw {$likes join $serves J | ungroup J | project quantity drinker | sort} \$likes \$serves
    tkshow $tw {$likes join $serves J | ungroup J | where {$(quantity) > 400}} \$likes \$serves
    #tkshow $tw {$likes join $serves J | ungroup J | extend daystogo:I {quantity/perday}} \$likes \$serves
}
gdemo [createDisplayWidget ""]
