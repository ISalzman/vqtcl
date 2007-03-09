# rainit.tcl -=- self-contained template for ratcl starlib setup

# %includeDir<../src>% <-- directives like these are processed by includer.tcl
# %includeDir<../noarch>%

# the following "preloads" information so that thrill.tcl doesn't read/source it
namespace eval ::thrill {
  namespace eval v {
    set init {
# %include<words1.th>%
# %include<>%
    }
  }
}

# %include<thrill.tcl>%
# %include<>%

proc ratclInit {} {
  rename ratclInit {}
  foreach x [info loaded] {
    switch -- [lindex $x 1] Ratcl { set self [lindex $x 0] }
  }
  # remember dirs view so ratcl.tcl could re-use it
  set thrill::radirs [thrill::vget [thrill::vopen $self] 0 dirs]
  set files [thrill::vget $thrill::radirs 0 2]
  foreach f {words2.th expr.db ratcl.tcl} {
    set n [lsearch -exact [thrill::vget $files * 0] $f]
    if {$n < 0} { error "$f is missing from $self" }
    foreach {name size date contents} [thrill::vget $files $n] break
    if {![string length $contents]} { error "empty $f" }
    if {$size != [string length $contents]} {
      set contents [thrill::vcall { nbuf swap xdez exec } BI $contents $size]
    }
    switch $f {
      words2.th { set ::thrill::v::init2 $contents }
      expr.db   { set ::thrill::v::egram $contents }
      ratcl.tcl { uplevel 1 $contents }
    }
  }
}

ratclInit
