--assert = print

require 'lvq'
assert(lvq._VERSION == "LuaVlerq 1.7")

-- the empty meta view
local emv = lvq.emv()
assert(#emv == 0)
assert(tostring(emv) == "view: view #0 SI()")

-- the meta-meta view
local mm = emv:meta()
assert(#mm == 3)
assert(tostring(mm) == "view: view #3 SI()")

-- row access
assert(tostring(mm[2]) == "row: 2 view SI()")
assert(mm[0][0] == "name")
assert(mm[1].name == "type")

assert(#vops.view(7) == 7)

print "OK"
