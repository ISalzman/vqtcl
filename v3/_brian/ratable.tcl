#! /usr/bin/env tclkit
lappend auto_path ..
source ~/other/people/brian_theado/tablelist.kit
source ~/other/people/brian_theado/snit.kit
package require tablelist
package require snit

package require Tk
package require ratcl
proc min {a b} {expr {$a < $b ? $a : $b}}
#
# TODO: Bind to <Configure> event to handle resizes
# TODO: Add [yview moveto] and [yview scroll] functionality and -yscrollcommand so scrollbars can be
#       supported
# TODO: Column width of zero means tablelist will automatically adjust with every insert/delete
#       which doesn't look good during scrolling.
# TODO: Handle subview display
#
snit::widget ratable {
    variable tl
    variable v
    variable tdr  ;# Top Display Row
    delegate option -height to tl
    constructor {view args} {
        set v $view

        # Add the column widths (0 means "auto") to the list of column names
        set cols [view width vdef 0 | product [view $v get @ | mapcols name] | get] 
        set tl [tablelist::tablelist $win.tl -columns $cols]
        $self configurelist $args
        pack $tl -expand 1 -fill both
        
        if {[vget $v #] <= [$tl cget -height]} {
            $tl insertlist 0 [view $v get *]
        } else {
            $tl insertlist 0 [view $v slice 0 [$tl cget -height] | get *]
        }
        set tdr 0
        $self createScrollBindings
    }
    # TODO: focus issues unless I do [focus $tl] 
    method createScrollBindings {} {
        bind [$tl bodytag] <Up> [list $self scrollup 1]
        bind [$tl bodytag] <Down> [list $self scrolldown 1]
        bind [$tl bodytag] <Prior> [list $self pageup 1]
        bind [$tl bodytag] <Next> [list $self pagedown 1]
    }
    method tl args {eval [linsert $args 0 $tl]}
    method scrolldown numlines {
        set h [$tl size]
        set numlines [min $numlines [expr [vget $v #] - ($tdr + $h)]]
        $tl delete 0 [expr $numlines - 1]
        incr tdr $numlines
        $tl insertlist end [view $v slice $tdr $h | last [min $numlines $h] | get *]
    }
    method pagedown numpages {
        $self scrolldown [expr [$tl size] * $numpages]
    }
    method scrollup numlines {
        set h [$tl size]
        set numlines [min $tdr $numlines]
        $tl delete [expr [$tl index end] - $numlines] end
        incr tdr -$numlines
        $tl insertlist 0 [view $v slice $tdr [min $numlines $h] | get *]
    }
    method pageup numpages {
        $self scrollup [expr [$tl size] * $numpages]
    }
}

#
# Unit test code
#
if { [info exists argv0] && \
            ![string compare [file tail [info script]] [file tail $argv0]] } {
package require tcltest
namespace import tcltest::*
testConstraint interactive 1

# Creates a view with $nr rows and $nc columns all cell values
# are equal to the row number
proc rectangle {nr nc} {
    view $nr to v
    view $nc tag x | reverse | loop {
        view $nr tag c$(x) | pair $v | to v
    }
    return $v
}
proc withWidget {winScript script} {
    set win [uplevel $winScript]
    pack $win -expand 1 -fill both
    if {[catch {uplevel $script} msg]} {
        destroy $win
        return -code error $msg
    } else {
        destroy $win 
        return $msg
    }
}
# Returns the column names the tablelist widget knows about
proc colnames r {
    view {width name alignment} vdef [$r tl cget -columns] | mapcols name | get
}
test 1 {empty view with a single column} {
    withWidget {ratable .r [rectangle 0 1] -height 7} {
        list [.r tl size] [colnames .r] 
    }
} {0 c0}
test 2 {single row view} {
    withWidget {ratable .r [rectangle 1 1] -height 7} {
        .r tl size
    }
} 1 
test 3 {view with multiple columns and rows} {
    withWidget {ratable .r [rectangle 3 3] -height 7} {
        list [.r tl size] [colnames .r]
    }
} {3 {c0 c1 c2}}
test 4 {view with more rows than the display} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r tl get 0 end
    }
} {0 1 2 3 4 5 6}
test 5 {Scroll down one row} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 1
        .r tl get 0 end
    }
} {1 2 3 4 5 6 7}
test 6 {Scroll down two rows} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 2
        .r tl get 0 end
    }
} {2 3 4 5 6 7 8}
test 7 {Scroll down twice} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 1
        .r scrolldown 1
        .r tl get 0 end
    }
} {2 3 4 5 6 7 8}
test 8 {Scroll down different height} {
    withWidget {ratable .r [rectangle 10 1] -height 5} {
        .r scrolldown 2
        .r tl get 0 end
    }
} {2 3 4 5 6}
test 9 {Attempt to scroll down past the end} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 10
        .r tl get 0 end
    }
} {3 4 5 6 7 8 9}
test 10 {Attempt to scroll down past the end twice} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 10
        .r scrolldown 10
        .r tl get 0 end
    }
} {3 4 5 6 7 8 9}
test 11 {Scroll up one} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 10
        .r scrollup 1
        .r tl get 0 end
    }
} {2 3 4 5 6 7 8}
test 12 {Scroll up two} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 10
        .r scrollup 2
        .r tl get 0 end
    }
} {1 2 3 4 5 6 7}
test 13 {Scroll up past the start} {
    withWidget {ratable .r [rectangle 10 1] -height 7} {
        .r scrolldown 10
        .r scrollup 20
        .r tl get 0 end
    }
} {0 1 2 3 4 5 6}
test 14 {Scroll down when display not full} {
    withWidget {ratable .r [rectangle 3 1] -height 7} {
        .r scrolldown 1
        .r tl get 0 end
    }
} {0 1 2}
test 15 {Page down one} {
    withWidget {ratable .r [rectangle 15 1] -height 5} {
        .r pagedown 1
        .r tl get 0 end
    }
} {5 6 7 8 9}
test 16 {Page down one} {
    withWidget {ratable .r [rectangle 15 1] -height 5} {
        .r pagedown 1
        .r tl get 0 end
    }
} {5 6 7 8 9}
test 17 {Page down 2} {
    withWidget {ratable .r [rectangle 15 1] -height 5} {
        .r pagedown 2
        .r tl get 0 end
    }
} {10 11 12 13 14}
test 18 {Page up 2} {
    withWidget {ratable .r [rectangle 15 1] -height 5} {
        .r pagedown 2
        .r pageup 2
        .r tl get 0 end
    }
} {0 1 2 3 4}
catch { console show }
cleanupTests
}
