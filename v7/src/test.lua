-- uncomment next line to show progress instead of silently checking results
--assert = print

require 'lvq'
assert(lvq._VERSION == "LuaVlerq 1.7", "version")

-- the empty meta view
local emv = view()
assert(#emv == 0, "emv row count")
assert(tostring(emv) == "view: view #0 SIV", "emv structure")

-- the meta-meta view
local mm = emv:meta()
assert(#mm == 3, "mm row count")
assert(tostring(mm) == "view: view #3 SIV", "mm structure")

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
assert(tostring(mm[2]) == "row: 2 view SIV", "row display")
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
assert(tostring(m1) == "view: view #3 SIV", "m1 display")

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

-- return view contents as a compact string
vopdef ('s', 'V', function (v)
  local m = v:meta()
  local x = {}
  for c = 1,#m do
    local z = m[c-1].name
    x[c] = (z == '' and '?') or z
  end
  local y = { table.concat(x, ' ') }
  for r = 1,#v do
    x = {}
    for c = 1,#m do
      local z = v[r-1][c-1] or ''
      x[c] = (type(z) == type(v) and '#'..#z) or tostring(z)
    end
    y[r+1] = table.concat(x, ' ')
  end
  return table.concat(y, '; ')
end)

assert(m2:s() == 'A B C;  0 0;  0 0', "m2 as string")

-- set all cells in new view
m2[0][0] = "abc"
m2[0][1] = 12
m2[0][2] = 34.56
m2[1].A = "cba"
m2[1].B = 21
m2[1].C = 65.43
assert(m2:s() == "A B C; abc 12 34.56; cba 21 65.43", "m2 dump after sets")

-- table to view conversions
assert(view{1,2,3}:s() == "?; 1; 2; 3", "simple list as 1-col int view")
assert(view{meta='A:I,B,C:D'; 1,'two',3.3,4,'five',6.6,7,'eight',9.9}:s() ==
        "A B C; 1 two 3.3; 4 five 6.6; 7 eight 9.9", "table as 3-col view")

-- callback views
assert(view(4):call("A:I", function (r) return r*r*r end):s() ==
        "A; 0; 1; 8; 27", "1-col callback view")
assert(view(4):call("A:I,B:I,C:I", function (r,c) return r*r+c end):s() ==
        "A B C; 0 1 2; 1 2 3; 4 5 6; 9 10 11", "3-col callback view")

-- row introspection
assert(#m1[1] == 1, "row index")
assert(tostring(m2[1]()) == "view: view #2 SID", "row view")

-- step views
assert(view(3):step():s() == "?; 0; 1; 2", "step iota")
assert(view(3):step(4):s() == "?; 4; 5; 6", "step with start")
assert(view(3):step(2,3):s() == "?; 2; 5; 8", "step with step")
assert(view(5):step(1,4,2):s() == "?; 1; 1; 5; 5; 9", "step with rate")
assert(view(3):step(2,0):s() == "?; 2; 2; 2", "step zero")
assert(view(3):step(2,-1):s() == "?; 2; 1; 0", "negative step")

-- row mapping
assert(view{3,4,5,6,7} [{2,4,0,2}]:s() == "?; 5; 7; 3; 5", "table as row map")
assert(view({3,4,5,6,7}) [view(3):step(4,-2)]:s() == "?; 7; 5; 3",
        "step as row map")

-- column mapping
assert((mm/1):s() == "type; 5; 1; 7", "one col by index")
assert((mm/'type'):s() == "type; 5; 1; 7", "one col by name")
assert((mm/{1,0,1}):s() == "type name type; 5 name 5; 1 type 1; 7 subv 7",
        "table as column map")

-- times vop
assert(view{5,6}:times(3):s() == "?; 5; 6; 5; 6; 5; 6", "times vop")

-- spread vop
assert(view{5,6}:spread(3):s() == "?; 5; 5; 5; 6; 6; 6", "times vop")

-- plus vop
assert((view{1,2,3}+view{9,8}):s() == "?; 1; 2; 3; 9; 8", "plus vop as +")
assert(view{1,2,3}:plus({6,5},{8,9}):s() == "?; 1; 2; 3; 6; 5; 8; 9",
        "plus vop with 3 views")

-- pair vop
assert((view{1,2,3}..view{4,5,6}):s() == "? ?; 1 4; 2 5; 3 6", "pair vop as ..")
assert(view{1,2,3}:pair(mm,{4,5,6}):s() ==
        "? name type subv ?; 1 name 5 #0 4; 2 type 1 #0 5; 3 subv 7 #0 6",
        "pair vop with 3 views")

-- product vop
assert(view{1,2}:product{3,4,5}:s() == "? ?; 1 3; 1 4; 1 5; 2 3; 2 4; 2 5",
        "product vop")

-- first vop
assert(view{1,2,3}:first(2):s() == "?; 1; 2", "first vop less")
assert(view{1,2,3}:first(4):s() == "?; 1; 2; 3", "first vop too many")

-- last vop
assert(view{1,2,3}:last(2):s() == "?; 2; 3", "last vop less")
assert(view{1,2,3}:last(4):s() == "?; 1; 2; 3", "last vop too many")

-- reverse vop
assert(view{1,2,3}:reverse():s() == "?; 3; 2; 1", "reverse vop")

-- clone vop
assert(m2:clone():s() == "A B C", "clone vop")

-- minimal range operations tests
assert(view{1,8}:r_flip(3,2):s() == "?; 1; 3; 5; 8", "r_flip")
assert(table.concat({view{1,2}:r_locate(1)}, '.') == "0.0", "r_locate")
assert(view{1,3}:r_insert(0,2,1):s() == "?; 0; 2; 3; 5", "r_insert")
assert(view{1,3,5,6}:r_delete(0,2):s() == "?; 0; 1; 3; 4", "r_delete")

-- minimal mutable operations tests
assert(not view(3):ismutable(), "ismutable")
assert(view(3):mutwrap():ismutable(), "mutwrap")
assert(mm:s() == mm:mutwrap():s(), "mutwrap identity")

-- rows iterator
local o = {}
for r in mm:rows() do table.insert(o, tostring(r)) end
assert(table.concat(o, ", ") ==
        "row: 0 view SIV, row: 1 view SIV, row: 2 view SIV", "rows iterator")

-- html rendering
--print(mm:html())
assert((mm:html():match("<table>.*</table>")), "html rendering")

-- mapped files and strings
assert(tostring(lvq.map("abcde", 1, 3)) == "bcd", "map substring")
local m = lvq.map("../etc/simple.db")
assert(#m == 61, "size of mapped file")
assert(#tostring(m) == 61, "size of mapped file as string")
assert(type(m) == "userdata", "map is userdata")
assert(tostring(lvq.map(m, 34, 8)) == "v[abc:I]", "substring in map")

-- map string/file to view
local d1 = lvq.open(m)
assert(tostring(d1) == "view: view #1 (I)", "open simple.db")
assert(#d1[0].v == 5, "size of subview in simple.db")
assert(tostring(d1[0].v) == "view: view #5 I", "subview in simple.db")
assert(d1[0].v:s() == "abc; 1; 22; 333; 4444; 55555", "dump view in simple.db")
local d2 = lvq.open(lvq.map("../etc/alltypes.db"))
assert(tostring(d2) == "view: view #1 (IIIIIIILFDSB)", "open alltypes.db")
assert(tostring(d2[0].v) == "view: view #5 IIIIIIILFDSB", "subview in alltypes.db")
assert(d2[0].v:s() ==
  "i0 i1 i2 i4 i8 i16 i32 l f d s b; "..
  "0 1 1 1 -127 -32767 1 111 1.5 11.1 s1 b11; "..
  "0 0 2 6 -106 -32746 22 222222 22.5 2222.1 s22 b2222; "..
  "0 1 1 13 -51 -32435 333 333333333 333.5 333333.1 s333 b333333; "..
  "0 0 0 12 -36 -28324 4444 444444444444 4444.5 44444444.1 s4444 b44444444; "..
  "0 1 3 3 -125 22787 55555 5.5555555555556e+14 55555.5 5555555555.1 "..
                                  "s55555 b5555555555", "row 0 of alltypes.db")
-- emit views
assert(#d1:emit() == 61, "emit simple.db size")
assert(#d2:emit() == 331, "emit alltypes.db size")
assert(#m2:emit() == 74, "emit test view size")
local s2 = d2:emit()
local l2 = lvq.open(lvq.map(s2,0,#s2))
assert(tostring(l2[0].v) == "view: view #5 IIIIIIILFDSB", "load emitted string")
assert(l2:s() == "v; #5", "check loaded emitted string")
assert(#l2[0].v:s() == 350, "check v contents size of loaded emitted string")

-- sorting
assert(view{9,5,3}:sortmap():s() == "?; 2; 1; 0", "sortmap on reversed ints")
assert(view{9,5,3}:sort():s() == "?; 3; 5; 9", "sort on reversed ints")
assert(mm:sortmap():s() == "?; 0; 2; 1", "sortmap multiple columns")

print "OK"
