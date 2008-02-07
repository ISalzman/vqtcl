package ifneeded vlerq 3 [list load [file join $dir libvlerq3[info shared]]]
package ifneeded ratcl 3 [list source [file join $dir ratcl.tcl]]
package ifneeded mklite 0.3 [list source [file join $dir mklite.tcl]]

package ifneeded vfs::mkcl 1.4 [list source [file join $dir mkclvfs.tcl]]
package ifneeded vfs::m2m 1.6 [list source [file join $dir m2mvfs.tcl]]
