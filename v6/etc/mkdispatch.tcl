#!/usr/bin/env tclkit
# Detect Web Preview mode (TextMate):
if {[lappend env(PWD)] eq "/" && [info exists env(TM_MODE)]} {
  catch { cd $env(TM_DIRECTORY) }
  puts {<font size=1><pre>}
}

# generate dispatch tables

set prefix [lindex $argv 0]
if {$prefix eq ""} {
    puts stderr "Usage: $argv0 prefix"
    exit 1
}

proc ctype {c} {
    string map {N nil I int L long F float D double \
                S string B bytes V view O objref T int} $c
}

proc gencall {name types} {
    set sep ""
    set i -1
    set call "  "
    set fieldmap {I o.a.i L w F o.a.f D d V o.a.v S o.a.s O o.a.p T o.a.i}
    if {[regexp {_(.+)} $types - va]} {
        set types [regsub {_.*} $types [string repeat V [string length $va]]]
    }
    foreach t [split $types ""] {
        if {$t eq ":"} {
            append call " = " $name "Vop("
            set sep ""
            set i -1
        } else {
            append call $sep "A\[[incr i]\]." [string map $fieldmap $t]
            set sep ","
        }
    }
    append call ");"
}

proc define {module vops} {
    global fdc defs
    if {$module ne "-"} {
        lappend defs "#ifdef VQ_${module}_H"
        puts $fdc ""
        puts $fdc "#ifdef VQ_${module}_H"
    }
    foreach {name types} $vops {
        set cmd ${name}Cmd_[string range $types 2 end]
        lappend defs "  { \"[string tolower $name]\", \"$types\", $cmd },"
        puts $fdc ""
        puts $fdc "static vq_Type $cmd (vq_Item A\[]) {"
        puts $fdc [gencall $name [string toupper $types]]
        puts $fdc "  return VQ_[ctype [string index $types 0]];"
        puts $fdc "}"
    }
    if {$module ne "-"} {
        lappend defs "#endif"
        puts $fdc ""
        puts $fdc "#endif"
    }
}

set fdh [open $prefix.h w]
puts $fdh "/* [file tail $prefix].h - generated code, do not edit */"
puts $fdh ""
puts $fdh "extern CmdDispatch f_[file tail $prefix]\[];"
puts $fdh ""
puts $fdh "/* end of generated code */"
close $fdh

set fdc [open $prefix.c w]
puts $fdc "/* [file tail $prefix].c - generated code, do not edit */"
puts $fdc ""
puts $fdc "#include \"vq_conf.h\""

define - {
    Empty       T:VII
    Meta        V:V
    Replace     V:VIIV
}

define LOAD {
    AsMeta      V:S
    Open        V:S
}

define MUTABLE {
    Mutable     V:V
}

define OPDEF {
    ColCat      V:_V
    ColMap      V:VV
    Pass        V:V
    RowCat      V:_V
    RowMap      V:VV
    Step        V:Iiiis
    Type        S:V
    View        V:Vv
}

puts $fdc ""
puts $fdh "CmdDispatch f_[file tail $prefix]\[] = {"
puts $fdc [join $defs \n]
puts $fdc "  {NULL,NULL,NULL}"
puts $fdc "};"
puts $fdc ""
puts $fdc "/* end of generated code */"
close $fdc
