-- uncomment next line to show progress instead of silently checking results
--assert = print

require 'lvq'
assert(lvq._VERSION == "LuaVlerq 1.7", "version")

-- the empty meta view
local emv = view()
assert(#emv == 0, "emv row count")
assert(tostring(emv) == "view: view #0 SI()", "emv structure")

-- the meta-meta view
local mm = emv:meta()
assert(#mm == 3, "mm row count")
assert(tostring(mm) == "view: view #3 SI()", "mm structure")

assert(mm[0][0] == "name", "mm name 0")
assert(mm[1][0] == "type", "mm name 1")
assert(mm[2][0] == "subv", "mm name 2")
assert(mm[0][1] == 5, "mm type 0")
assert(mm[1][1] == 1, "mm type 1")
assert(mm[2][1] == 7, "mm type 2")
assert(mm[0][2] == emv, "mm subv 0")
assert(mm[1][2] == emv, "mm subv 1")
assert(mm[2][2] == emv, "mm subv 2")

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
local m1 = view("A:S,B:I,C:D")
assert(#m1 == 3, "m1 row count")
assert(tostring(m1) == "view: view #3 SI()", "m1 display")

assert(m1[0][0] == "A", "m1 name 0")
assert(m1[1][0] == "B", "m1 name 1")
assert(m1[2][0] == "C", "m1 name 2")
assert(m1[0][1] == 5, "m1 type 0")
assert(m1[1][1] == 1, "m1 type 1")
assert(m1[2][1] == 4, "m1 type 2")
assert(m1[0][2] == emv, "m1 subv 0")
assert(m1[1][2] == emv, "m1 subv 1")
assert(m1[2][2] == emv, "m1 subv 2")
assert(m1:dump() == [[
  name  type  subv
  ----  ----  ----
  A        5    #0
  B        1    #0
  C        4    #0]], "m1 dump")

-- create a fresh view
local m2 = view(2,"A:S,B:I,C:D")
assert(#m2 == 2, "m2 row count")
assert(tostring(m2) == "view: view #2 SID", "m2 display")
assert(m2:dump() == [[
  A  B  C
  -  -  -
     0  0
     0  0]], "m2 initial dump")

-- set all cells in new view
m2[0][0] = "abc"
m2[0][1] = 12
m2[0][2] = 34.56
m2[1].A = "cba"
m2[1].B = 21
m2[1].C = 65.43
assert(m2:dump() == [[
  A    B   C    
  ---  --  -----
  abc  12  34.56
  cba  21  65.43]], "m2 dump after sets")

-- table to view conversions
assert(view{1,2,3}:dump() == [[
  ?
  -
  1
  2
  3]], "simple list as 1-col int view")
assert(view{meta='A:I,B,C:D'; 1,'two',3.3,4,'five',6.6,7,'eight',9.9}:dump()==[[
  A  B      C  
  -  -----  ---
  1  two    3.3
  4  five   6.6
  7  eight  9.9]], "table as 3-col view")

-- callback views
assert(view(4):call("A:I", function (r) return r*r*r end):dump() == [[
  A 
  --
   0
   1
   8
  27]], "1-col callback view")
assert(view(4):call("A:I,B:I,C:I", function (r,c) return r*r+c end):dump() == [[
  A  B   C 
  -  --  --
  0   1   2
  1   2   3
  4   5   6
  9  10  11]], "3-col callback view")

-- row introspection
assert(#m1[1] == 1, "row index")
assert(tostring(m2[1]()) == "view: view #2 SID", "row view")

-- step views
assert(view(3):step():dump() == [[
  ?
  -
  0
  1
  2]], "step iota")
assert(view(3):step(4):dump() == [[
  ?
  -
  4
  5
  6]], "step with start")
assert(view(3):step(2,3):dump() == [[
  ?
  -
  2
  5
  8]], "step with step")
assert(view(5):step(1,4,2):dump() == [[
  ?
  -
  1
  1
  5
  5
  9]], "step with rate")
assert(view(3):step(2,0):dump() == [[
  ?
  -
  2
  2
  2]], "step zero")
assert(view(3):step(2,-1):dump() == [[
  ?
  -
  2
  1
  0]], "negative step")

-- row mapping
assert(view({3,4,5,6,7}) [{2,4,0,2}]:dump() == [[
  ?
  -
  5
  7
  3
  5]], "table as row map")
assert(view({3,4,5,6,7}) [view(3):step(4,-2)]:dump() == [[
  ?
  -
  7
  5
  3]], "step as row map")

-- column mapping
assert((mm/1):dump() == [[
  type
  ----
     5
     1
     7]], "one col by index")
assert((mm/'type'):dump() == [[
  type
  ----
     5
     1
     7]], "one col by name")
assert((mm/{1,0,1}):dump() == [[
  type  name  type
  ----  ----  ----
     5  name     5
     1  type     1
     7  subv     7]], "table as column map")

-- times vop
assert(view({5,6}):times(3):dump() == [[
  ?
  -
  5
  6
  5
  6
  5
  6]], "times vop")

-- spread vop
assert(view({5,6}):spread(3):dump() == [[
  ?
  -
  5
  5
  5
  6
  6
  6]], "times vop")

print "OK"
