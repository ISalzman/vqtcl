#! /usr/bin/env tclkit

# generate a simple Metakit data file

package require Mk4tcl

cd [file dirname [info script]]

mk::file open db simple.db
mk::view layout db.v abc:I

foreach x {1 22 333 4444 55555} {
    mk::row append db.v abc $x
}

mk::file commit db
