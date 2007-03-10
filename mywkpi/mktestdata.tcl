#!/Users/jcw/w/kitgen/8.4/kit-small/tclkit-cli 

source ../../vlerq/src_tcl/ratcl.tcl
package require ratcl

view {A B C} def {1 2 3 a b c} | save test.vqdb
