# initests.tcl -=- common code included at start of all *.test files

package require Tcl 8.4

package require tcltest 2.2
namespace import tcltest::*
eval configure $argv

testsDirectory [file dirname [info script]]

# assume tests are run from the top level or from a subdir right under it

foreach x {. build ../build ?} {
  if {$x eq "?"} { return -code error "can't find thrive[info sharedlibext]" }
  if {![catch { load $x/thrive[info sharedlibext] }]} break
}

# always make the working directory the one where thrive was loaded from,
# because this is were test files and test executables are expected to be
# (or else we can reach whatever we need with ../{noarch,src,tests}/blah)

cd $x

foreach x $uses {
  source ../src/$x.tcl
  catch { namespace import -force ${x}::* }
}

unset x

testConstraint memchan [expr {[catch { close [::thrill::do_memchan] }] == 0}]
testConstraint metakit [expr {[catch { package require Mk4tcl }] == 0}]
testConstraint vfs [expr {[catch { package require vfs }] == 0}]

# fails if this is run before thrill.tcl has been sourced
catch { testConstraint xbit [thrill::vcall hasXbit] }

set exeSuff ""
if {[testConstraint win]} { set exeSuff .exe }

testConstraint vq [file exists vq$exeSuff]
testConstraint vqx [file exists vqx$exeSuff]
