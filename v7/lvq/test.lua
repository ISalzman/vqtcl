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

assert(tostring(view(7)) == "view: view #7 ", "zero-column view")
assert(#view(7) == 7, "zero-column view row count")

-- dump operator
assert(emv:dump() == [[
  name  type  subv
  ----  ----  ----]], "emv dump")
assert(mm:dump() == [[
  name  type  subv
  ----  ----  ----
  name     5    #0
  type     1    #0
  subv     7    #0]], "mm dump")
assert(view(7):dump() == "  (7 rows, 0 columns)", "zero-column dump")

-- p operator (result not checked)
--mm:p()

-- create meta-view from string
local m1 = view(3,"A:S,B:I,C:F")
assert(#m1 == 3, "m1 row count")

--[=[
assert(tostring(m1) == "view: view #0 SIF", "m1 display")

assert(m1:dump() == [[
  name  type  subv
  ----  ----  ----]], "m1 dump")
--]=]

print "OK"
