#!/usr/bin/env tclkit
# exthdr -=- extract the extension header from thrive.h

set copy 0
while {[gets stdin line] >= 0} {
  if {[regexp {%ext-on%} $line]} { set copy 1; continue }
  if {[regexp {%ext-off%} $line]} { set copy 0; continue }
  regsub -all {\m([A-Z]\w*|[a-z]+\w*[A-Z]\w*)\M} $line {t4_\1} line
  if {$copy} { puts $line }
}
