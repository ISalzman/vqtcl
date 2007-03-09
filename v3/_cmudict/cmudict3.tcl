load ./libvlerq3.dylib
source ../library/ratcl.tcl
#load /src/vlerq1/vlerq1.dll
#source /src/vlerq1/library/ratcl.tcl

proc partitionWordSounds sounds {
    set prevType {};
    set output {}; set curList {}
    set vowelSounds {
        AA AE AH AO AW AY
        EH ER EY IH IY
        OW OY UH UW
    }
    foreach sound $sounds {
        set curType [expr {[lsearch -sorted -exact $vowelSounds $sound] >= 0}]
        if {($curType == $prevType) || [string length $prevType] == 0} {
            lappend curList $sound
        } else {
            lappend output $curList
            set curList $sound
        }
        set prevType $curType
    }
    if {[llength $curList] > 0} {lappend output $curList}
    return $output
}
proc cat file {
    set fd [open $file]
    return [read $fd]
}
# Remove extra information from the frequent words data
proc extractFrequentWords fw {
    return [regsub -line -all {^[0-9]+ ([^%]+)%.+$} $fw {\1}]
}
# Somewhat arbitrary list of words that are in cntlist that I want to exclude
proc excludeWords {} {
    return [view word vdef {
    ab abed ad aide ass axe b bali beau belle berg bern bloc blond bonn boyle
    c cant canter cantle clique
    cock comer commie crone cur dais damn dane dec descendent dewar disc distal dope dross
    dun dyeing ell ester fe fete finn franc g
    gallus gauche gauss gee girt grille harry hater hell
    inn j jan jap jew john johnny joss joule k kent ketch l lager lao
    luger lustre ma'am maine mamma mantel mid mien mike mil min n naval navel os
    pap papal pier pith pro psi
    rancor rape raped reich rite rome ruck scot scud sec
    si sic sioux sis sol spain spic
    stein steppe strait sward swart swiss tarry tat taut teat thoreau
    tit torr tory trestle troupe un v verve vicar watt weir wile
    writ
    }]
}

# Convert cmudict contents into list alternating between word and its sound
proc getSimilarWordDict dict {
    # Similar word dictionary
    set swd {}
    foreach line [split $dict \n] {
        if {[regexp {^[A-Z]} $line]} {
            set word [lindex $line 0]
            
            # The rest of the line is a list of sounds for the word
            # Remove the numbers--those are emphasis indicators for multi-syllable words
            # Consider only words with only alphabetic characters
            if {[regexp {[^A-Za-z]} $word]} continue
            
	    #set sounds [regsub -all {[0-2]} [lrange $line 1 end] {}]
	    set sounds [string map {0 "" 1 "" 2 ""} [lrange $line 1 end]]
	    set sounds [partitionWordSounds $sounds]
	    
	    set n [llength $sounds]
	    for {set x 0} {$x < $n} {incr x} {
		lappend swd $word [lreplace $sounds $x $x ?]
	    }
        }
    }
    return [string tolower $swd]
}
# pipeline operator that will time execution.  Uses vget to force delazification
ratcl::vopdef vtime {v args} {
    set disp $args
    switch [lindex $disp 0] {
        join - join_i {
            # Avoid string conversion of view argument when displaying
            set disp [concat [lindex $disp 0] _view_ [lrange $args 2 end]]
        }
    }
    puts [time {
        # tried uplevel here, but kept getting 'std_stringer: Assertion `0' failed'.  Haven't tried since well before the July 11 vlerq code
        set v [eval [linsert $args 0 view $v]]
        puts "[vget $v #] result rows:"
    }]:'$disp'
    #puts [view $v dump]
    return $v
}

proc create {cmudictFile wordnetFrequentWords} {
	# Parse out the frequent words from the wordnet file
    puts "reading and parsing frequent words"
	puts fw:[time {set fwlist [extractFrequentWords [cat $wordnetFrequentWords]]}]

	# Parse out the cmudict into word/sound pairs.  Convert to lower case.
	# Add entries for each possible sound wildcard
    puts "reading and parsing cmudict"
	puts cmu:[time {set cmudictList [getSimilarWordDict [cat $cmudictFile]]}]
    
    # force to lists now, to avoid skewing the following timings
    llength $fwlist
    llength $cmudictList

    view - debug {
	# Create the initial views and exclude the words from
	# cmudict that aren't in the frequent words list
	use word
	vdef $fwlist 
	unique 
	except [excludeWords]
	to fw
	use {word pattern}
	vdef $cmudictList
	join_i $fw
	to sw
	# Gather all the similar words together
        join_i [view $sw rename word similar]
        where {$(word) != $(similar)}
        group word pattern similars
        where {[vget $(similars) #] > 2}
        group word patterns
        where {[vget $(patterns) #] > 1}
	# Save result view to file
	group words
	save similarword.mk
    }
    return
}
create cmudict cntlist

interp alias {} db {} view [view similarword.mk open] get 0

puts [db words | dump 5]
puts [db words | where {$(word) == "cat"} | ungroup patterns | ungroup similars | dump 5]

view - debug {
  use similarword.mk
  open
  get 0 words
  where {$(word) == "cat"}
}
