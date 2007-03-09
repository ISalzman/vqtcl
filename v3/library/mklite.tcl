# mklite.tcl -- Compatibility layer for Metakit

# 0.3 modified to use the vlerq extension i.s.o. thrive

package provide mklite 0.3
package require vlerq

namespace eval mklite {
  # the mkdb array maps open db handles to view references
  variable mkdb   

# call emulateMk4tcl to define compatibility aliases in a specified namespace
# if the MKLITE_DEBUG env var is defined, all calls and returns will be traced
  proc emulateMk4tcl {{ns ::mk}} {
    foreach x {channel cursor file get loop select view} {
      # NOTE: clobbers existing mk::* commands, might want to rename 'em first?
      if {[info exists ::env(MKLITE_DEBUG)]} {
        interp alias {} ${ns}::$x {} ::mklite::debug ::mklite::$x
      } else {
        interp alias {} ${ns}::$x {} ::mklite::$x
      }
    }
  }
  proc debug {args} {
    puts ">>> [list $args]"
    set r [uplevel 1 $args]
    set s [regsub -all {[^ -~]} $r ?]
    puts " << [string length $r]: [string range $s 0 49]"
    return $r
  }
  proc mk_obj {path} {
    variable mkdb
    # memoize last result (without row number), it's likely to be used again
    set pre [regsub {!\d+$} $path {}]
    if {$mkdb(?) eq $pre} { return $mkdb(:) }
    set n 0
    foreach {x y} [regsub -all {[:.!]} :$path { & }] {
      switch -- $x {
        : { set r $mkdb($y) }
        ! { set n $y }
        . { set r [vget $r $n $y] }
      }
    }
    set mkdb(?) $pre
    set mkdb(:) $r
    return $r
  }
# partial emulation of the mk::* commands
  proc file {cmd db args} {
    variable mkdb
    set file [lindex $args 0]
    switch $cmd {
      open { set mkdb(?) ""; set mkdb($db) [::view $file mapf]; return $db }
      close { set mkdb(?) ""; unset mkdb($db); return }
      views { return [vget $mkdb($db) @ * 0] }
    }
    error "mkfile $cmd?"
  }
  proc view {cmd path args} {
    set a1 [lindex $args 0]
    switch $cmd {
      info {
        return [vget [mk_obj $path] @ * 0]
      }
      layout {
        set layout "NOTYET"
        if {[llength $args] > 0 && $layout != $a1} {
          #error "view restructuring not supported"
        }
        return $layout
      }
      size {
        set len [vget [mk_obj $path] #]
        if {[llength $args] > 0 && $len != $a1} {
          error "view resizing not supported"
        }
        return $len
      }
      default { error "mkview $cmd?" }
    }
  }
  proc cursor {cmd cursor args} {
    upvar $cursor v
    switch $cmd {
      create {
        NOTYET
      }
      incr {
        NOTYET
      }
      pos -
      position {
        if {$args != ""} {
          unset v
          regsub {!-?\d+$} $v {} v
          append v !$args
          return $args
        }
        if {![regexp {\d+$} $v n]} {
          set n -1
        }
        return $n
      }
      default { error "mkcursor $cmd?" }
    }
  }
  proc get {path args} {
    set vw [mk_obj $path]
    set row [regsub {^.*!} $path {}]
    set sized 0
    if {[lindex $args 0] eq "-size"} {
      incr sized
      set args [lrange $args 1 end]
    }
    set ids 0
    if {[llength $args] == 0} {
      incr ids
      set args [vget $vw @ * 0]
    }
    set r {}
    foreach x $args {
      if {$ids} { lappend r $x }
      set v [vget $vw $row $x]
      if {$sized} { set v [string length $v] }
      lappend r $v
    }
    if {[llength $args] == 1} { set r [lindex $r 0] }
    return $r
  }
  proc loop {cursor path args} {
    upvar $cursor v
    #unset -nocomplain v ;# avoids errors if v already exists as array
    if {[llength $args] == 0} {
      set args [list $path]
      set path $v
      regsub {!-?\d+$} $path {} path
    }
    set first [lindex $args 0]
    set last [lindex $args 1]
    switch [llength $args] {
      1 { set first 0
          set limit [vget [mk_obj $path] #]
          set step 1 }
      2 { set limit [vget [mk_obj $path] #]
          set step 1 }
      3 { set step 1 }
      4 { set step [lindex $args 2] }
      default { error "mkloop arg count?" }
    }
    set body [lindex $args end]
    set code 0
    for {set i $first} {$i < $limit} {incr i $step} {
      set v $path!$i
      set code [catch [list uplevel 1 $body] err]
      switch $code {
        1 -
        2 { return -code $code $err }
        3 { break }
      }
    }
  }
  proc select {path args} {
    set vw [mk_obj $path]
    set n [vget $vw #]
    set r {}
    for {set i 0} {$i < $n} {incr i} { lappend r $i }
    set n [llength $args]
    for {set i 0} {$i < $n} {incr i} {
      set glob 0
      switch -- [lindex $args $i] {
        -count { set count [lindex $args [incr i]]; continue }
        -glob  { incr glob; incr i }
      }
      set c [vget $vw * [lindex $args $i]]
      set k [lindex $args [incr i]]
      set r2 {}
      foreach x $r {
        set v [lindex $c $x]
        if {$glob} {
          if {[string match $k $v]} { lappend r2 $x }
        } else {
          if {$k eq $v}             { lappend r2 $x }
        }
      }
      set r $r2
    }
    if {[info exists count]} {
      set r [lrange $r 0 [incr count -1]]
    }
    return $r
  }
  proc channel {path name mode} {
    package require vfs ;# TODO: needs vfs, could use "chan create" in 8.5
    if {$mode ne "r"} { error "mkchannel? mode $mode" }
    set fd [vfs::memchan]
    fconfigure $fd -translation binary
    puts -nonewline $fd [get $path $name]
    seek $fd 0
    return $fd
  }
}
