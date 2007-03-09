#!/usr/bin/env tclkit
# mkblocked -=- create a test datafile for blocked access

file delete mkblk.db
mk::file open db mkblk.db
mk::view layout db.bv {{_B {k1 k2:I}}}
mk::view open db.bv rawview
set data [rawview view blocked]

for {set i 0} {$i < 10000} {incr i} {
  $data insert end k1 s_$i k2 $i
}

rename $data ""
rename rawview ""

mk::file close db
