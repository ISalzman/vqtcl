set dir [file join $dir library]
source [file join $dir pkgIndex.tcl]
set dir [file dirname $dir]

# override the installation setting for uninstalled use
package ifneeded vlerq  1 \
  [list load [file join $dir libvlerq1[info sharedlibext]]]
