package provide ratclcmd 0.4
package require ratcl
set cmd ratcl
foreach file [glob -dir [file dirname [info script]] *.tcl] {
    switch -glob $file [info script] - *pkgIndex.tcl {} default {
        source $file
        lappend cmd [file tail [file rootname $file]]
    }
}
if {[info exists tkcon]} {
    puts stderr "loaded [join $cmd {, }]"

	array set ratcltxt {
		collect		1		compare		1	compat		1	debug		1
		debugv		1		dump 		1	emit		1	exceptmap	1
		get 		1		hashcol		1	hashview	1	html		1
		isectmap	1		loop		1	max			1	min			1
		names 		1		ratk 		1	refs		1	rowcmp		1
		roweq		1		rowhash		1	save		1	size 		1
		sortmap		1		structdesc	1	structure	1	type		1
		unionmap	1		uniquemap	1	width		1	write		1
	}

	proc ratclfilter {code res} {
		if {[string trim $res] eq {}} { return "" }
		if {$code} { return [ratclerr $res] }
		set cmd [history event 0]
		set first [lindex $cmd 0]
		set alias [lindex [interp alias {} $first] 0]
		if {$first ne "view" && $alias ne "view"} {
			return [ratclerr $res]
		}
		set ops [split $cmd |]
		if {[llength $ops] eq 1} {
			# single operation
			set ops [lindex $ops 0]
			if {[lindex $ops 0] eq "view"} {
				set pos 2	;# view cmd
			} else {
				set pos 1	;# alias to view cmd
			}
			set last [lindex [split $ops] $pos]
		} else {
			# pipeline
			set last [lindex [split [string trim [lindex $ops end]]] 0]
		}
		if {[info exists ::ratcltxt($last)]} {
			return [ratclerr $res]
		}
		if {[catch {set size [view $res size]} err]} {
			if {[string first vlerq $err] != -1} {
				return [ratclerr $err]
			}
			if {$err eq "row index out of range"} { return $err }
			return [string range $res 0 100]
		}
		return $size
	}

	proc ratclerr {err} {
		# change the vlerq error so it better suits ratcl
		set err [string map {"vlerq " ""} $err]
		regsub " view " $err " " err
		return $err
	}

	tkcon resultfilter ratclfilter

	tkcon linelength 10000

}
