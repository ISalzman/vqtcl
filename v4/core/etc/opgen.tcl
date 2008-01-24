#!/usr/bin/env tclkit
# Detect Web Preview mode (TextMate):
if {[lappend env(PWD)] eq "/" && [info exists env(TM_MODE)]} {
  catch { cd $env(TM_DIRECTORY) }
  puts {<font size=1><pre>}
}

proc : {name in out {def ""}} {
  set id [string tolower $name]
  set ::defs($id) [list $out $in $def $name]
  if {[info exists ::only]} {
    set ::include($::only,$id) $name
    set ::exclude($id) $name
  }
}
proc tcl {cmds} {
  set ::only tcl
  uplevel 1 $cmds
  unset ::only
}
proc python {cmds} {
  set ::only python
  uplevel 1 $cmds
  unset ::only
}
proc ruby {cmds} {
  set ::only ruby
  uplevel 1 $cmds
  unset ::only
}
proc objc {cmds} {
  set ::only objc
  uplevel 1 $cmds
  unset ::only
}
proc lua {cmds} {
  set ::only lua
  uplevel 1 $cmds
  unset ::only
}
proc unsafe {args} {
  foreach x $args {
    set ::unsafecmds([string tolower $x]) $x
  }
}
proc prepare {} {
  global names defs
  set names [lsort -dict [array names defs]]
}
proc fieldname {type} {
  string map {C .c I .i L .w F .d D .d S .s B .s \
              V .v i .c s .c n .c N .i O .o} $type
}
proc fieldtype {type} {
  string map {C Column I int L int64_t F float D double S {const char*} \
              V View_p i Column s Column n Column N int O Object_p} $type
}
proc itemtype {out} {
  set last [string index $out end]
  set type [string map {I int S string V view C column U unknown} $last]
  return IT_$type
}
proc gen-headers {} {
  set o1 {}
  set o2 {}
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident} continue
    #if {[llength $def] == 0} continue

    if {[llength $def] == 1} {
      set r {}
      foreach x [split $in ""] {
        lappend r [fieldtype $x]
      }
      if {$r eq ""} { set r void }
      lappend o1 "[format %-6s [fieldtype $out]] ($def) ([join $r {, }]);"
    }

    append o2 "ItemTypes (${ident}Cmd_$in) (Item_p a);\n"
  }
  emit src/wrap_gen.h "[join [lsort -index 1 $o1] \n]\n\n$o2"
}
proc gen-simple {} {
  set o {}
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident} continue
    if {[llength $def] != 1} continue

    if {$def eq ""} { set def $ident }

    set i 0
    set r {}
    foreach x [split $in ""] {
      lappend r "a\[$i][fieldname $x]"
      incr i
    }
    set avec [join $r ", "]

    set last [string index $out end]

    append o "
ItemTypes ${ident}Cmd_$in (Item_p a) {
  a\[0][fieldname $last] = ${def}($avec);
  return [itemtype $out];
}
"
  }
  return $o
}
proc gen-compound {} {
  set o {}
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident} continue
    if {[string first " " $def] < 0} continue

    append o "
/* : $name ( $in-$out ) $def ; */
ItemTypes ${ident}Cmd_$in (Item_p a) {
"
    if {$def != " "} {
      append o "  ItemTypes t;\n"
    }
    
    set l [string length $in]
    foreach d $def {
      if {[string is int $d]} {
        append o "  a\[$l].i = $d;\n"
        incr l
      } else {
        if {![info exists ::defs($d)]} {
          error "$name: can't find definition of '$d'"
        }
        foreach {out2 in2 def2 ident2} $::defs($d) break
        incr l -[string length $in2]
        if {$l < 0} {
          error "$name: stack underflow with call to '$d'"
        }
        set a a
        if {$l > 0} { append a + $l }
        if {$d eq $ident2} {
          foreach x {0 1 2 3 4} {
            set $x "a\[[expr {$l+$x}]]"
          }
          append o "  /* $d $l */ [subst -nocommands $def2]\n"
        } else {
          set x [expr {$l + [string length $out2]}]
          append o "  t = ${ident2}Cmd_${in2}($a);\n"
        }
        incr l [string length $out2]
      }
    }
    if {[incr l -1] > 0} {
      append o "  a\[0] = a\[$l];\n"
    }
    set u [itemtype $out]
    if {$u eq "IT_unknown"} {
      set u t
    }
    append o "  return $u;
}
"
  }
  return $o
}
proc gen-tcl-index {input} {
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(tcl,$name)])} continue
    incr n
  }
  set o "/* $n definitions generated for Tcl:\n$input*/\n\n"
  set n 0
  foreach safe {0 1} {
    foreach name $::names {
      if {[info exists ::unsafecmds($name)] == $safe} continue
      foreach {out in def ident} $::defs($name) break
      if {$name eq $ident || ([info exists ::exclude($name)] &&
                              ![info exists ::include(tcl,$name)])} continue
      append o [format {  { %-13s %-11s %-18s },} \
                          \"$name\", \"$out:$in\", ${ident}Cmd_$in] \n
      incr n
    }
    if {!$safe} {
      append o "#define UNSAFE $n\n"
    }
  }
  emit src_tcl/opdefs_gen.h $o
}
proc gen-ruby-index {input} {
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(ruby,$name)])} continue
    incr n
  }
  set o "/* $n definitions generated for Ruby:\n$input*/\n\n"
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(ruby,$name)])} continue

    append o [format {  { %-13s %-11s %-18s },} \
                        \"$name\", \"$out:$in\", ${ident}Cmd_$in] \n
    incr n
  }
  emit src_ruby/vlerq_ext.h $o
}
proc gen-objc-index {input} {
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(objc,$name)])} continue
    incr n
  }
  set o "/* $n definitions generated for Objective-C:\n$input*/\n\n"
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(objc,$name)])} continue

    append o [format {  { %-13s %-11s %-18s },} \
                        \"$name\", \"$out:$in\", ${ident}Cmd_$in] \n
    incr n
  }
  emit src_objc/vlerq_ext.h $o
}
proc gen-lua-index {input} {
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(lua,$name)])} continue
    incr n
  }
  set o "/* $n definitions generated for Lua:\n$input*/\n\n"
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(lua,$name)])} continue

    append o [format {  { %-13s %-11s %-18s },} \
                        \"$name\", \"$out:$in\", ${ident}Cmd_$in] \n
    incr n
  }
  emit src_lua/vlerq_ext.h $o
}
proc gen-python-index {input} {
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(python,$name)])} continue
    incr n
  }
  set o "/* $n definitions generated for Python:\n$input*/\n\n"
  set n 0
  foreach name $::names {
    foreach {out in def ident} $::defs($name) break
    if {$name eq $ident || ([info exists ::exclude($name)] &&
                            ![info exists ::include(python,$name)])} continue

    append o [format {  { %-13s %-11s %-18s },} \
                        \"$name\", \"$out:$in\", ${ident}Cmd_$in] \n
    incr n
  }
  emit src_python/vlerq_ext.h $o
}
proc emit {name text} {
  set path [file normalize [file dirname [info script]]/../$name]
  set text "/* [file tail $name] - generated code, do not edit */\n
[string trim $text]\n
/* end of generated code */"
  if {[info script] eq ""} {
    puts $text
  } else {
    set fd [open $path w]
    puts $fd $text
    close $fd
  }
  puts "$path: [llength [split $text \n]] lines"
}
proc gen-wrappers {} {
  set o {
#include <stdlib.h>

#include "intern.h"
#include "wrap_gen.h"
  }
  append o [gen-simple] [gen-compound]
  emit src/wrap_gen.c $o
}
proc generate {input} {
  uplevel $input
  prepare
  gen-headers
  gen-tcl-index $input
  gen-ruby-index $input
  gen-objc-index $input
  gen-python-index $input
  gen-lua-index $input
  gen-wrappers
  #puts $errorInfo
}

generate {
  # name       in     out    inline-code
  : drop       U      {}     {}
  : dup        U      UU     {$1 = $0;}
  : imul       II     I      {$0.i *=  $1.i;}
  : nip        UU     U      {$0 = $1;}
  : over       UU     UUU    {$2 = $0;}
  : rot        UUU    UUU    {$3 = $0; $0 = $1; $1 = $2; $2 = $3;}
  : rrot       UUU    UUU    {$3 = $2; $2 = $1; $1 = $0; $0 = $3;}
  : swap       UU     UU     {$2 = $0; $0 = $1; $1 = $2;}
  : tuck       UU     UUU    {$1 = $2; $0 = $1; $2 = $0;}
  
  tcl {
    # name       in    out
    : Def        OO     V
    : Deps       O      O
    : Emit       V      O
    : EmitMods   V      O
    : Get        VX     U
    : Load       O      V
    : LoadMods   OV     V
    : Loop       X      O
    : MutInfo    V      O
    : Ref        OX     O
    : Refs       O      I
    : Rename     VO     V
    : Save       VS     I
    : To         OO     O
    : Type       O      O
    : View       X      O
  }
  
  python {
    # name       in    out
  }
  
  ruby {
    # name       in    out
    : AtRow      OI     O
  }
  
  lua {
    # name       in    out
  }
  
  objc {
    # name       in    out
    : At         VIO    O
  }
  
  # name       in    out
  : BitRuns    i      C
  : Data       VX     V
  : Debug      I      I
  : HashCol    SO     C
  : Max        VN     O
  : Min        VN     O
  : Open       S      V
  : ResizeCol  iII    C
  : Set        MIX    V
  : StructDesc V      S
  : Structure  V      S
  : Sum        VN     O
  : Write      VO     I

  # name       in    out  internal-name
  : Blocked    V      V   BlockedView
  : Clone      V      V   CloneView
  : ColMap     Vn     V   ColMapView
  : ColOmit    Vn     V   ColOmitView
  : Coerce     OS     C   CoerceCmd
  : Compare    VV     I   ViewCompare
  : Compat     VV     I   ViewCompat
  : Concat     VV     V   ConcatView
  : CountsCol  C      C   NewCountsColumn
  : CountView  I      V   NoColumnView
  : First      VI     V   FirstView
  : GetCol     VN     C   ViewCol
  : Group      VnS    V   GroupCol
  : HashFind   VIViii I   HashDoFind
  : Ijoin      VV     V   IjoinView
  : GetInfo    VVI    C   GetHashInfo
  : Grouped    ViiS   V   GroupedView
  : HashView   V      C   HashValues
  : IsectMap   VV     C   IntersectMap
  : Iota       I      C   NewIotaColumn
  : Join       VVS    V   JoinView
  : Last       VI     V   LastView
  : Mdef       O      V   ObjAsMetaView
  : Mdesc      S      V   DescToMeta
  : Meta       V      V   V_Meta
  : OmitMap    iI     C   OmitColumn
  : OneCol     VN     V   OneColView
  : Pair       VV     V   PairView
  : RemapSub   ViII   V   RemapSubview
  : Replace    VIIV   V   ViewReplace
  : RowCmp     VIVI   I   RowCompare
  : RowEq      VIVI   I   RowEqual
  : RowHash    VI     I   RowHash
  : Size       V      I   ViewSize
  : SortMap    V      C   SortMap
  : Step       VIIII  V   StepView
  : StrLookup  Ss     I   StringLookup
  : Tag        VS     V   TagView
  : Take       VI     V   TakeView
  : Ungroup    VN     V   UngroupView
  : UniqueMap  V      C   UniqMap
  : ViewAsCol  V      C   ViewAsCol
  : Width      V      I   ViewWidth
               
  # name       in    out  compound-definition
  : Append     VV     V   {over size swap insert}
  : ColConv    C      C   { }
  : Counts     VN     C   {getcol countscol}
  : Delete     VII    V   {0 countview replace}
  : Except     VV     V   {over swap exceptmap remap}
  : ExceptMap  VV     C   {over swap isectmap swap size omitmap}
  : Insert     VIV    V   {0 swap replace}
  : Intersect  VV     V   {over swap isectmap remap}
  : NameCol    V      V   {meta 0 onecol}
  : Names      V      C   {meta 0 getcol}
  : Product    VV     V   {over over size spread rrot swap size repeat pair}
  : Repeat     VI     V   {over size imul 0 1 1 step}
  : Remap      Vi     V   {0 -1 remapsub}
  : Reverse    V      V   {dup size -1 1 -1 step}
  : Slice      VIII   V   {rrot 1 swap step}
  : Sort       V      V   {dup sortmap remap}
  : Spread     VI     V   {over size 0 rot 1 step}
  : Types      V      C   {meta 1 getcol}
  : Unique     V      V   {dup uniquemap remap}
  : Union      VV     V   {over except concat}
  : UnionMap   VV     C   {swap exceptmap}
  : ViewConv   V      V   { }
  
  # some operators are omitted from restricted execution environments
  unsafe Open Save
}
