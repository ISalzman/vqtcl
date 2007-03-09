#! /usr/bin/env tclkit
lappend auto_path ..
package require ratcl

# Creates a view with $nr rows and $nc columns all cell values
# are equal to the row number
proc rectangle {nr nc} {
    view $nr to v
    view $nc tag x | reverse | loop {
        view $nr tag c$(x) | pair $v | to v
    }
    return $v
}

puts [view [rectangle 5 10] dump]

proc rect2 {nr nc} {
    view $nr tag x | get | to range
    view $nc collect { $range } | to data
   #view $nc collect { "c${(#)}:I" } | vdef {expand}$data
    eval [linsert $data 0 view $nc collect { "c${(#)}:I" } | vdef]
}

puts 1
puts [view [rect2 5 10] dump]

proc rect3 {nr nc} {
    view $nr to v
    view $nc collect { [view $nr tag c$(#)] } | to cols
   #view $v pair {expand}$cols
    eval [linsert $cols 0 view $v pair]
}

puts 2
puts [view [rect3 5 10] dump]
