# Wrapper around orasql that adds the oracle error to the error message
proc myorasql {sh sql} {
    if {[set code [catch {orasql $sh $sql} result]]} {
        set ::errorInfo "[oramsg $sh error]\n at character offset [oramsg $sh peo]\nsql:$sql\n$::errorInfo"
        return -code $code -errorinfo $::errorInfo -errorcode $::errorCode [oramsg $sh error]
    } else {
        return $result
    }
}

# Column names and types.  Assume all NUMBER are ints until the data is seen
proc getColsView sh {
    # Lower case column names and don't allow ':' in the name
    set colnames [view name def [string map {: .} [string tolower [oracols $sh name]]]]

    # Get the column types
    set types {}
    foreach type [oracols $sh type] {
        if {$type == "NUMBER"} {
            lappend types I
        } else {
            lappend types S
        }
    }
    return [view $colnames pair [view type def $types]]
}

# Given an open oracle statement handle and a sql query, returns
# a ratcl view with the query results.
# The first row of the matrix is the column headers
proc orasql2View {sh sql} {
    package require Oratcl 4.0
    package require ratcl
    myorasql $sh $sql
    orafetch $sh -datavariable row
    set cols [getColsView $sh]
    set data {}
    while {[oramsg $sh rc] == 0} {
        set colnum 0
        foreach cell $row {
            if {![string is integer $cell]} {
                if {[view $cols get $colnum type] == "I"} {
                    if {[string is double $cell]} {
                        set cols [view $cols set $colnum type F]
                    } else {
                        set cols [view $cols set $colnum type S]
                    }
                }
            }
            incr colnum
        }
        eval lappend data $row
        orafetch $sh -datavariable row
    }
    set colInfo [view $cols collect {"$(name):$(type)"}]
    return [view $colInfo def $data] 
}
ratcl::vopdef oratcl {sh args} {
    # Allow for both braced and unbraced sql
    if {[llength $args] > 1} {
        orasql2View $sh [join $args { }]
    } else {
        orasql2View $sh [lindex $args 0]
    }
}
