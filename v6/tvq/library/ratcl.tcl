# ratcl.tcl -- Wrapper around the tvq extension

package provide ratcl [package require tvq]

proc luas {s} { lvq "o" [lvq "gs" loadstring $s] }  ;# evaluate string in lua
luas { package.loaded['lvq.core'] = lvq }           ;# emulate module setup
lvq "ggsc" rawset _G tcl ""                         ;# define a 'tcl' callback

# Examples:
#   luas { print('Hello!') }
#   luas { tcl('puts','Hi!') }

# emulate a require, but from a specific file
luas { loadfile('../lvq/src/lvq.lua')('lvq') }
