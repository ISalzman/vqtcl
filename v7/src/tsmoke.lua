--[[  Basic smoke tests for LuaVlerq, called from test.lua wrapper.
      $Id$
      This file is part of Vlerq, see src/vlerq.h for full copyright notice.
--]]

-- the empty meta view
local emv = vq()
assert(#emv == 0, "emv row count")
assert(tostring(emv) == "view: view #0 SIV", "emv structure")

-- the meta-meta view
local mm = emv:meta()
assert(#mm == 3, "mm row count")
assert(tostring(mm) == "view: view #3 SIV", "mm structure")

assert(mm[1][1] == "name", "mm name 1")
assert(mm[2][1] == "type", "mm name 2")
assert(mm[3][1] == "subv", "mm name 3")
assert(mm[1][2] == vq.S, "mm type 1")
assert(mm[2][2] == vq.I, "mm type 2")
assert(mm[3][2] == vq.V, "mm type 3")
assert(mm[1][3] == emv, "mm subv 1")
assert(mm[2][3] == emv, "mm subv 2")
assert(mm[3][3] == emv, "mm subv 3")

-- row access
assert(tostring(mm[3]) == "row: 3 view SIV", "row display")
assert(mm[1][1] == "name", "column access by index")
assert(mm[2].name == "type", "columns access by name")

assert(tostring(vq(7)) == "view: view #7 ", "zero-column view")
assert(#vq(7) == 7, "zero-column view row count")

-- dump operator
assert(emv:dump() == [[
  name  type  subv
  ----  ----  ----]], "emv dump")
assert(mm:dump() == [[
  name  type  subv
  ----  ----  ----
  name     6    #0
  type     1    #0
  subv     8    #0]], "mm dump")
assert(vq(7):dump() == "  (7 rows, 0 columns)", "zero-column dump")

-- p operator (result not checked)
--mm:p()

-- create metaview from string
local m1 = vq("A:S,B:I,C:D")
assert(#m1 == 3, "m1 row count")
assert(tostring(m1) == "view: view #3 SIV", "m1 display")

assert(m1[1][1] == "A", "m1 name 1")
assert(m1[2][1] == "B", "m1 name 2")
assert(m1[3][1] == "C", "m1 name 3")
assert(m1[1][2] == vq.S, "m1 type 1")
assert(m1[2][2] == vq.I, "m1 type 2")
assert(m1[3][2] == vq.D, "m1 type 3")
assert(m1[1][3] == emv, "m1 subv 1")
assert(m1[2][3] == emv, "m1 subv 2")
assert(m1[3][3] == emv, "m1 subv 3")
assert(m1:dump() == [[
  name  type  subv
  ----  ----  ----
  A        6    #0
  B        1    #0
  C        5    #0]], "m1 dump")

-- create a fresh view
local m2 = vq(2,"A:S,B:I,C:D")
assert(#m2 == 2, "m2 row count")
assert(tostring(m2) == "view: view #2 SID", "m2 display")
assert(m2:dump() == [[
  A  B  C
  -  -  -
     0  0
     0  0]], "m2 initial dump")

assert(m2:s() == 'A B C;  0 0;  0 0', "m2 as string")

-- set all cells in new view
m2[1][1] = "abc"
m2[1][2] = 12
m2[1][3] = 34.56
m2[2].A = "cba"
m2[2].B = 21
m2[2].C = 65.43
assert(m2:s() == "A B C; abc 12 34.56; cba 21 65.43", "m2 dump after sets")

-- table to view conversions
assert(vq{1,2,3}:s() == "?; 1; 2; 3", "simple list as 1-col pos view")
assert(vq{meta='A:I,B,C:D'; 1,'two',3.3,4,'five',6.6,7,'eight',9.9}:s() ==
        "A B C; 1 two 3.3; 4 five 6.6; 7 eight 9.9", "table as 3-col view")
  
print "End tsmoke"
