# ratcl.tcl -- Wrapper around the tvq extension

package provide ratcl [package require tvq]

interp alias {} lua {} tvq dostring         ;# evaluate string in lua
lua { package.loaded['lvq.core'] = lvq }    ;# emulate module setup

# Examples:
#   lua { print('Hello!') }
#   lua { tcl('puts','Hi!') }

# emulate a require, but from a specific file
lua { loadfile('../lvq/src/lvq.lua')('lvq') }
