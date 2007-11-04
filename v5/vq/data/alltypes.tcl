#! /usr/bin/env tclkit

# generate a Metakit data file containing all possible data types

package require Mk4tcl

cd [file dirname [info script]]

file delete alltypes.db
mk::file open db alltypes.db
mk::view layout db.v {i0:I i1:I i2:I i4:I i8:I i16:I i32:I l:L f:F d:D s:S b:B}

foreach x {1 22 333 4444 55555} {
    mk::row append db.v \
        i0 0 i1 [expr {$x%2}] i2 [expr {$x%4}] i4 [expr {$x%16}] \
        i8 [expr {$x%256-128}] i16 [expr {$x%65536-32768}] i32 $x \
        l $x$x$x f $x.5 d $x$x.1 s s$x b b$x$x
}

mk::file commit db
