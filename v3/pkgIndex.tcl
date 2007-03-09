set dir [file join $dir library]
source [file join $dir pkgIndex.tcl]
set dir [file dirname $dir]

# override the installation setting for uninstalled use
package ifneeded vlerq 3 \
  [list load [file join $dir libvlerq3[info sharedlibext]]]
