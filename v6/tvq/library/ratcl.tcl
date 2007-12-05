# ratcl.tcl -- Wrapper around the tvq extension

package provide ratcl [package require tvq]

interp alias {} luas {} tvq "gs" dostring
luas { function setglobal(x,y) _G[x]= y end }
tvq "gsc" setglobal tcl "" ;# empty cmd prefix, but still a callback
