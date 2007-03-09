#!/usr/bin/env wish

load ../libvlerq3[info sharedlibext]
lappend auto_path ..
source ratable2.tcl

#view {A B} vdef {a b 1 2 a c} | group A G | tkr
view 1000 collect {int(rand()*10)} | asview Rand:I | tag Index | group Rand Count | sort | tkr
view 1000 collect {int(rand()*10)} | asview Rand:I | group Rand Count | sort | tkr
#view /Users/jcw/Sites/www.equi4.com/pub/sk/kitten.kit open | get 0 dirs | tkr
