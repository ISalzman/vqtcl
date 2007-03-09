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
proc main {} {
    #puts [time {source mydata.tcl}]
    puts A0-[time {source /Users/Shared/Datasets/bt/mydata.tcl}]
    puts A1-[time {llength $cmudictList}]
#   set cmudictList [string range $cmudictList 0 10000]
    puts B0-[time {set sw [view {word pattern} vdef $cmudictList]}]
    puts C0-[time {puts [vget $sw #]}]
    puts D1-[time {set fw [view word vdef $fwlist]}]
    puts D2-[time {puts [vget $fw #]}]
#   puts S1-[time {
#     view $sw group sw | pair [view $fw group fw] | save /tmp/cmu.db
#   }]
#   puts S2-[file size /tmp/cmu.db]
    puts D3-[time {set fw [view $fw except [excludeWords]]}]
    puts E0-[time {puts [vget $fw #]}]
    #puts F-[time {set sw [view $sw join_i $fw]}]
puts S0-[view $sw structure]-[view $sw names]
    puts F1-[time {set sw [view $sw join $fw _J]}]
    puts F2-[time {puts [vget $sw #]}]
puts S1-[view $sw structure]-[view $sw names]
    puts F3-[time {set sw [view $sw ungroup -1]}]
    puts G0-[time {puts [vget $sw #]}]
puts S2-[view $sw structure]-[view $sw names]
    puts H0-[time {set sw [view $sw group pattern similar]}]
    puts I0-[time {puts [vget $sw #]}]
    puts J1-[time {set sw [view $sw where {[vget $(similar) #] > 2}]}]
    puts J2-[time {puts [vget $sw #]}]
    puts K0-[time {puts [view $sw dump]}]
puts K1-[view $sw structure]-[view $sw names]
puts K2-[view $sw get 0 similar | names]
}
#load libvlerq1.so
load ./libvlerq3.dylib
source ../library/ratcl.tcl
main

if 0 {
  # on biggie (P4 1G PowerBook), 28-06-2006
  A0-1284818 microseconds per iteration
  B0-6254107 microseconds per iteration
  667358
  C0-522 microseconds per iteration
  D1-260652 microseconds per iteration
  36974
  D2-421 microseconds per iteration
  D3-826 microseconds per iteration
  36825
  E0-224972 microseconds per iteration
  F1-90 microseconds per iteration
  667358
  F2-1401503 microseconds per iteration
  F3-62 microseconds per iteration
  140570
  G0-31381631 microseconds per iteration
  H0-59 microseconds per iteration
  125757
  I0-31909395 microseconds per iteration
  J1-3711112 microseconds per iteration
  1614
  J2-371 microseconds per iteration
    pattern  similar
    -------  -------
    ? s           #6
    ey ?         #35
    ? k           #6
    ? {k t}       #9
    ae ?         #29
    ? d          #17
    ? jh         #19
    ah ? iy       #4
    ? m          #12
    ? r          #19
    eh ?         #49
    eh r ?        #7
    ? l          #15
    ay ?          #7
    ao ?         #12
    ae l ?        #3
    ? l aw        #6
    ah ? aw       #6
    ah l ?        #6
    ao ? er       #3
    .......  .......
  K0-5638 microseconds per iteration
}
