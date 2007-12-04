# used by TextMate for F7 on bookie

set mode lvq

switch $mode {
    lvq {
        cd [file dirname [info script]]/../lvq
        set e [catch { exec make install 2>@stderr } r]
        puts ""
        if {$e} { puts $r; exit 1 }
        exec tests/test.lua >@stdout 2>@stderr
    }
    tvq {
        cd [file dirname [info script]]/../tvq
        exec make test >@stdout 2>@stderr
    }
}