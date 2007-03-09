# lite.tcl -=- self-contained template for tclkit lite

# %includeDir<../src>% <-- directives like these are processed by includer.tcl
# %includeDir<../noarch>%

load {} thrive

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

#proc ::history args {} ;# for debugging when kitlite fails very early on

proc tclkitInit {} {
  rename tclkitInit {}
  # remember dirs view so boot.tcl can re-use it
  set thrill::dirs [thrill::vget [thrill::vopen [info nameofexe]] 0 dirs]
  set files [thrill::vget $thrill::dirs 0 2]
  set n [lsearch -exact [thrill::vget $files * 0] boot.tcl]
  if {$n < 0} { error "no boot.tcl file" }
  foreach {name size date contents} [thrill::vget $files $n] break
  if {![string length $contents]} { error "empty boot.tcl" }
  catch {load {} zlib}
  if {$size != [string length $contents]} {
    set contents [zlib decompress $contents]
  }
  uplevel #0 $contents
  if {$::tcl_platform(platform) eq "windows"} {
    package ifneeded dde 1.2 {load {} dde}
    package ifneeded registry 1.0 {load {} registry}
  }
}

tclkitInit
