package require starkit
if {[starkit::startup] == "sourced"} return
package require app-tkcon

if {[llength $argv] == 1 && [file readable [set db [lindex $argv 0]]]} {
    package require vfs::util
    package require vfslib
    package require ratclcmd
    wm withdraw .
    view $db open | ratk
    tkwait window .ratk1
    exit
} else {
    # jcw, 01-12-2003: needed to force tkcon to start inside starkit
    eval ::tkcon::Init $argv

    slave eval "package require vfs::util"
    slave eval "package require vfslib"

    ::tkcon::EvalCmd $::tkcon::PRIV(console) {
        set tkcon 1
        package require ratclcmd
    }
}
