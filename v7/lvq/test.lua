-- uncomment next line to show progress instead of silently checking results
--assert = print

require 'lvq'
assert(lvq._VERSION == "LuaVlerq 1.7", "version")

-- the empty meta view
local emv = lvq.emv()
assert(#emv == 0, "emv row count")
assert(tostring(emv) == "view: view #0 SI()", "emv structure")

-- the meta-meta view
local mm = emv:meta()
assert(#mm == 3, "mm row count")
assert(tostring(mm) == "view: view #3 SI()", "mm structure")

-- row access
assert(tostring(mm[2]) == "row: 2 view SI()", "row display")
assert(mm[0][0] == "name", "column access by index")
assert(mm[1].name == "type", "columns access by name")

assert(tostring(vops.view(7)) == "view: view #7 ", "zero-column view")
assert(#vops.view(7) == 7, "zero-column view row count")

print "OK"
