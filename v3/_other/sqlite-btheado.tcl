source sqlite-3_3_5.kit  ;# sqlite.org offers this kit with Windows, linux and Mac binaries
package require sqlite3

# Adapted from ratcl's m_dump
proc dump {v {maxrows 20}} {
 set data [view $v first $maxrows | get]
 set colnames [colnames $v]
 set numcols [llength $colnames]
 if {$numcols == 0} return
 set numrows [expr [llength $data] / $numcols]

 # Calculate column widths
 for {set col 0} {$col < $numcols} {incr col} {
   set w [string length [lindex $colnames $col]]
   for {set row 0} {$row < $numrows} {incr row} {
     set idx [expr ($row * $numrows) + $col]
     set cell [lindex $data $idx]
     if {[string length $cell] > $w} {set w [string length $cell]}
   }
   if {$w > 50} {set w 50}
   append fmt "  " %-$w.${w}s
   append hdr "  " [format %-${w}s [lindex $colnames $col]]
   append bar "  " [string repeat - $w]
 }

 # Add the header and a formatted version of each row to the output
 set r [list $hdr $bar]
 for {set row 0} {$row < $numrows} {incr row} {
   set cmd [list format $fmt]
   foreach cell [lrange $data [expr $row*$numcols] [expr ($row+1)*$numcols - 1]] {
     lappend cmd [regsub -all {[^ -~]} $cell .]
   }
   lappend r [eval $cmd]
 }

 # Add footer dots if the entire view was not displayed
 set fullrowcount [view $v select count(*) | get]
 if {$fullrowcount > $maxrows} {lappend r [string map {- .} $bar]}
 join $r \n
}

# Adapted from ratcl's m_html
proc html v {
 set names [colnames $v]
 set o <table>
 append o {<style type="text/css"><!--\
   table {border-spacing:0; border:1px solid #aaa; margin:0 0 2px 0}\
   th {background-color:#eee; font-weight:normal}\
   td {vertical-align:top}\
   th,td {padding:0 2px}\
   th.row,td.row {color:#aaa; font-size:75%}\
 --></style>}
 append o \n <tr>
 foreach x $names { append o <th> $x </th> }
 append o </tr>\n
 view $v each c {
   append o <tr>
   foreach x $names {
       set z [string map {& &amp\; < &lt\; > &gt\;} $c($x)]
       append o {<td align="right">}
       append o $z </td>
   }
   append o </tr>\n
 }
 append o </table>\n
}

# TODO: should take optional db as input? And the table variable
# should be a counter stored in a database table?
proc vdef args {
   variable table
   if {![info exists table]} {
       sqlite [namespace current]::db :memory:
       set table 0
   } else {
       incr table
   }

   # Adapted from ratcl's vdef
   set data [lindex $args end]
   set args [lrange $args 0 end-1]
   set d [llength $data]
   set c [llength $args]
   if {$d > 0} {
       if {$c == 0} { error "no args defined" }
       if {$d%$c != 0} { error "data is not an exact number of rows" }
       set n [expr {$d/$c}]
   } else {
       set n 0
   }

   # Create the sqlite table and insert the data
   db eval "create table t$table ([join $args ,])"
   foreach $args $data {
       set row {}
       foreach col $args {
           lappend row [set $col]
       }
       db eval "insert into t$table values ([join $row ,])"
   }

   # A basic view is just a list of the sqlite db and a table name
   return [list [namespace current]::db t$table]
}
proc vopen {db table} {return [list $db $table]}
proc freeze v {
   variable table
   set db [lindex $v 0]
   incr table
   $db eval "create table t$table as [createQuery $v]"
   return [list $db t$table]
}

# Adapted from ratcl's m_do
proc do {v cmds} {
 set r [list view $v]
 foreach x [split $cmds \n] {
   if {![regexp {^\s*#} $x]} { append r " | " $x }
 }
 uplevel 1 $r
}

# Retreive column names for the given view
proc colnames v {
   view $v each r break
   return $r(*)
}
proc renamecols {v colmap} {
   set cols [colnames $v]
   array set renames $colmap
   set newcols {}
   foreach col $cols {
       if {[info exists renames($col)]} {
           lappend newcols [list $col $renames($col)]
       } else {
           lappend newcols $col
       }
   }
   set query "select [join $newcols ,]"
}

proc omitcols {v omit} {
   set cols [colnames $v]
   set newcols {}
   foreach col $cols {
       if {[lsearch -exact $omit $col] == -1} {
           lappend newcols $col
       }
   }
   set query "select [join $newcols ,]"
}

# TODO: vfun, save?
# vfun - sqlite has a subcommand "function" that could accomplish something similar
# insert - allow multiple rows by using the same format as for vdef?
# commit - sqlite autocommits unless the sql is surrounded by begin/commit
proc view {v args} {
   while {[llength $args]} {
       set n [lsearch -exact $args |]
       if {$n < 0} { set n [llength $args]; lappend args | }
       set cmdAndArgs [lreplace $args $n end]
       set db [lindex $v 0]
       set table [lindex $v 1]
       if {[llength $cmdAndArgs]} {
           switch [lindex $cmdAndArgs 0] {
               select - where - order -
               join - union - intersect - except -
               first - concat - last {
                   # Derived view operations--just add them to the list--
                   # they will be processed when createQuery is called
                   lappend v $cmdAndArgs
               }
               rename {
                   lappend v [renamecols $v [lrange $cmdAndArgs 1 end]]
               }
               omitcols {
                   lappend v [omitcols $v [lrange $cmdAndArgs 1 end]]
               }
               clone {
                  set v [eval vdef [colnames $v] {{}}]
               }
               get {set v [$db eval [createQuery $v]]}
               each {
                   set aname [lindex $cmdAndArgs 1]
                   set body [lindex $cmdAndArgs 2]
                   set v [uplevel [list $db eval [createQuery $v] $aname $body]]
               }
               tosql {
                   set v [createQuery $v]
               }
               do {set v [do $v [lindex $cmdAndArgs 1]]}
               freeze {set v [freeze $v]}
               open {
                   set v [vopen $db [lindex $cmdAndArgs 1]]
               }
               info {
                   switch [lindex $cmdAndArgs 1] {
                       names {set v [colnames $v]}
                       db {set v $db}
                       table {set v $table}
                       default {error "'[lindex $cmdAndArgs 1]' should be one of names,db,table"}
                   }
               }
               dump {
                 if {[llength $cmdAndArgs] == 2} {
                   set v [dump $v [lindex $cmdAndArgs 1]]
                 } else {
                   set v [dump $v]
                 }
               }
               html {set v [html $v]}
               insert {
                   if {[llength $v] > 2} {set v [freeze $v]}
                   # Insert the "into table" part of the delete statement and execute
                   set sql [concat insert into $table [lrange $cmdAndArgs 1 end]]
                   $db eval $sql
               }
               delete {
                   if {[llength $v] > 2} {set v [freeze $v]}
                   # Insert the "from table" part of the delete statement and execute
                   set sql [concat delete from $table [lrange $cmdAndArgs 1 end]]
                   $db eval $sql
               }
               set {
                   if {[llength $v] > 2} {set v [freeze $v]}
                   # Insert the "table" part of the update statement and execute
                   set sql [concat update $table [lrange $cmdAndArgs 0 end]]
                   $db eval $sql
               }
               default {
                   error "Invalid subcommand: '[lindex $cmdAndArgs 0]'"
               }
           }
       }
       set args [lreplace $args 0 $n]
   }
   return $v
}

proc createQuery v {
   set db [lindex $v 0]
   set table [lindex $v 1]
   set query "select * from $table"  ;# return full table if commands are empty
   foreach cmd [lrange $v 2 end] {
       # Preprocess some special cases
       switch [lindex $cmd 0] {
           concat {
               set cmd [list {union all} [lindex $cmd 1]]
           }
           first {
               set cmd [list limit [lindex $cmd 1]]
           }
       }
       switch [lindex $cmd 0] {
           select {
               # The select can have anything except for the from clause
               # Insert the from clause just before the first "where", "group",
               # or "order" clause or at the end if no clauses found
               if {![regsub {where|group|order} $cmd "from $table &" query]} {
                   set query [concat $cmd from $table]
               }
           }
           where - order - limit {
               set query [concat select * from $table $cmd]
           }
           last {
               set limit [lindex $cmd 1]
               set totalrows [db eval [concat select count(*) from $table]]
               set offset [expr $totalrows - $limit]
               set query [concat select * from $table limit $limit offset $offset]
           }
           join {
               # First argument is the view to join with, so just do an lreplace of that
               # arg with createQuery called on that arg
               set cmd [join [lreplace $cmd 1 1 ([createQuery [lindex $cmd 1]])] " "]
               set query [concat select * from $table $cmd]
           }
           intersect - union - except - "union all" {
               # First argument is the view to join with, so just do an lreplace of that
               # arg with createQuery called on that arg
               set cmd [join [lreplace $cmd 1 1 [createQuery [lindex $cmd 1]]] " "]
               set query [concat select * from $table $cmd]
           }
           default {error "invalid cmd [lindex $cmd 0]"}
       }
       set table ($query)
   }
   return $query
}
if 0 { # tests
set v [vdef a b {1 2 3 4 5 6}]
puts [view $v get]
puts [view $v select a+b | get]
puts [view $v union [vdef a b {7 8 9 10} ] | get]
puts [view $v select a,b,a-b | tosql]
puts [view $v order by b desc | get]
puts [view $v insert values (7,8) | get]
puts [view $v set a=55 where a=1 | get]
puts [view $v delete where a=55 | get]
puts [view $v select a,b where a=5 order by b | dump]
puts [view $v html]
puts [view $v do {
   select a,b,a-b order by b desc
   first 1
   dump
   }]
puts [view $v clone | insert values (1,2) | dump]
puts [view $v open sqlite_master | dump]
}