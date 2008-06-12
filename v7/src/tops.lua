--[[  More operator tests for LuaVlerq, called from test.lua wrapper.
      $Id$
      This file is part of Vlerq, see src/vlerq.h for full copyright notice.
--]]

local emv = vq()
local mm = emv:meta()
local m1 = vq("A:S,B:I,C:D")
local m2 = vq(2,"A:S,B:I,C:D")

-- calculated views
assert(vq(4):calc("A:I", function (r) return r*r*r end):s() ==
        "A; 1; 8; 27; 64", "1-col calculated view")
assert(vq(4):calc("A:I,B:I,C:I", function (r,c) return r*r+c end):s() ==
        "A B C; 2 3 4; 5 6 7; 10 11 12; 17 18 19", "3-col calculated view")

-- row introspection
assert(#m1[2] == 2, "row index")
assert(tostring(m2[2]()) == "view: view #2 SID", "row view")

-- step views
assert(vq(3):step():s() == "?; 1; 2; 3", "step iota")
assert(vq(3):step(4):s() == "?; 4; 5; 6", "step with start")
assert(vq(3):step(2,3):s() == "?; 2; 5; 8", "step with step")
assert(vq(5):step(1,4,2):s() == "?; 1; 1; 5; 5; 9", "step with rate")
assert(vq(3):step(2,0):s() == "?; 2; 2; 2", "step zero")
assert(vq(3):step(2,-1):s() == "?; 2; 1; 0", "negative step")

-- row mapping
assert(vq{3,4,5,6,7}[{3,5,1,3}]:s() == "?; 5; 7; 3; 5", "table as row map")
assert(vq{3,4,5,6,7}[vq(3):step(5,-2)]:s() == "?; 7; 5; 3",
        "step as row map")

-- column mapping
assert((mm/2):s() == "type; 6; 1; 8", "one col by index")
assert((mm/'type'):s() == "type; 6; 1; 8", "one col by name")
assert((mm/{2,1,2}):s() == "type name type; 6 name 6; 1 type 1; 8 subv 8",
        "table as column map")

-- times vop
assert(vq{5,6}:times(3):s() == "?; 5; 6; 5; 6; 5; 6", "times vop")

-- spread vop
assert(vq{5,6}:spread(3):s() == "?; 5; 5; 5; 6; 6; 6", "spread vop")

-- plus vop
assert((vq{1,2,3}+vq{9,8}):s() == "?; 1; 2; 3; 9; 8", "plus vop as +")
assert(vq{1,2,3}:plus({6,5},{8,9}):s() == "?; 1; 2; 3; 6; 5; 8; 9",
        "plus vop with 3 views")

-- pair vop
assert((vq{1,2,3}..vq{4,5,6}):s() == "? ?; 1 4; 2 5; 3 6", "pair vop as ..")
assert(vq{1,2,3}:pair(mm,{4,5,6}):s() ==
        "? name type subv ?; 1 name 6 #0 4; 2 type 1 #0 5; 3 subv 8 #0 6",
        "pair vop with 3 views")

-- product vop
assert(vq{1,2}:product{3,4,5}:s() == "? ?; 1 3; 1 4; 1 5; 2 3; 2 4; 2 5",
        "product vop")

-- first vop
assert(vq{1,2,3}:first(2):s() == "?; 1; 2", "first vop less")
assert(vq{1,2,3}:first(4):s() == "?; 1; 2; 3", "first vop too many")

-- last vop
assert(vq{1,2,3}:last(2):s() == "?; 2; 3", "last vop less")
assert(vq{1,2,3}:last(4):s() == "?; 1; 2; 3", "last vop too many")

-- reverse vop
assert(vq{1,2,3}:reverse():s() == "?; 3; 2; 1", "reverse vop")

-- clone vop
assert(m2:clone():s() == "A B C", "clone vop")

-- minimal range operations tests
assert(vq{1,8}:r_flip(3,2):s() == "?; 1; 3; 5; 8", "r_flip")
assert(table.concat({vq{1,2}:r_locate(1)}, '.') == "1.1", "r_locate")
assert(vq{2,4}:r_insert(1,2,1):s() == "?; 1; 3; 4; 6", "r_insert")
assert(vq{2,4,6,7}:r_delete(1,2):s() == "?; 1; 2; 4; 5", "r_delete")
--assert(vq{1,4}:r_delete(3,1):s() == "?; 1; 2; 3; 4", "r_delete inside")

-- minimal mutable operations tests
assert(not vq(3):ismutable(), "ismutable")
assert(vq(3):mutwrap():ismutable(), "mutwrap")
assert(mm:s() == mm:mutwrap():s(), "mutwrap identity")

-- rows iterator
local o = {}
for r in mm:rows() do table.insert(o, tostring(r)) end
assert(table.concat(o, ", ") ==
        "row: 1 view SIV, row: 2 view SIV, row: 3 view SIV", "rows iterator")

-- html rendering
assert(mm:html():match("<table>.*</table>") and true, "html rendering")

-- mapped files and strings
assert(tostring(vq.map("abcde", 2, 3)) == "bcd", "map substring")
local m = vq.map("../etc/simple.db")
assert(#m == 61, "size of mapped file")
assert(#tostring(m) == 61, "size of mapped file as string")
assert(type(m) == "userdata", "map is userdata")
assert(tostring(vq.map(m, 35, 8)) == "v[abc:I]", "substring in map")

-- map string/file to view
local d1 = vq.open("../etc/simple.db")
assert(tostring(d1) == "view: subview #1 (I)", "open simple.db")
assert(#d1[1].v == 5, "size of subview in simple.db")
assert(tostring(d1[1].v) == "view: subview #5 I", "subview in simple.db")
assert(d1[1].v:s() == "abc; 1; 22; 333; 4444; 55555", "dump view in simple.db")
local d2 = vq.open("../etc/alltypes.db")
assert(tostring(d2) == "view: subview #1 (IIIIIIILFDSB)", "open alltypes.db")
assert(tostring(d2[1].v) == "view: subview #5 IIIIIIILFDSB", "subview in alltypes.db")
assert(d2[1].v:s() ==
  "i0 i1 i2 i4 i8 i16 i32 l f d s b; "..
  "0 1 1 1 -127 -32767 1 111 1.5 11.1 s1 b11; "..
  "0 0 2 6 -106 -32746 22 222222 22.5 2222.1 s22 b2222; "..
  "0 1 1 13 -51 -32435 333 333333333 333.5 333333.1 s333 b333333; "..
  "0 0 0 12 -36 -28324 4444 444444444444 4444.5 44444444.1 s4444 b44444444; "..
  "0 1 3 3 -125 22787 55555 5.5555555555556e+14 55555.5 5555555555.1 "..
                                  "s55555 b5555555555", "row 0 of alltypes.db")

-- emit and load views
assert(#d1:emit() == 61, "emit simple.db size")
assert(#d2:emit() == 331, "emit alltypes.db size")
assert(#m2:emit() == 59, "emit test view size")
local l2 = vq.load(d2:emit())
assert(tostring(l2[1].v) == "view: subview #5 IIIIIIILFDSB",
  "load emitted string")
assert(l2:s() == "v; #5", "check loaded emitted string")
assert(#l2[1].v:s() == 350, "check v contents size of loaded emitted string")

-- sorting
assert(vq{9,5,3}:sortmap():s() == "?; 3; 2; 1", "sortmap on reversed ints")
assert(vq{9,5,3}:sort():s() == "?; 3; 5; 9", "sort on reversed ints")
assert(mm:sortmap():s() == "?; 1; 3; 2", "sortmap multiple columns")

-- view comparisons
assert(mm == mm, "compare same views")
assert(mm ~= m2, "compare different views")
assert(vq{9,5,3} == vq{9,5,3}, "compare views with same contents")
assert(vq{9,5,3} ~= vq{9,5,2}, "compare views with different contents")
assert(vq{} == vq{}, "compare empty views")
assert(vq{1} <= vq{1}, "compare v <= v")
assert(vq{1} >= vq{1}, "compare v >= v")
assert(vq{1} < vq{2}, "compare v1 < v2")
assert(vq{} < vq{1}, "compare v1 < v2, no rows in v1")
assert(vq{1} < vq{1,2}, "compare v1 < v2, more rows in v2")
assert(vq{2} > vq{1}, "compare v2 > v1")

-- row comparisons
local r = mm[2]
assert(r == r, "compare same rows")
assert(r <= r, "compare r <= r")
assert(r >= r, "compare r >= r")
assert(r <= mm[2], "compare r <= r, but as different objects")
assert(r >= mm[2], "compare r >= r, but as different objects")
assert(r == mm[2], "compare same rows, but as different objects")
assert(r ~= mm[3], "compare different rows")
assert(vq{9,5,3}[3] == vq{9,5,3}[3], "compare rows with same contents")
assert(vq{9,5,3}[3] ~= vq{9,5,2}[3], "compare rows with different contents")

-- unique and set operations
assert(vq{1,2,3,2,2,4}:uniquemap():s() == "?; 1; 2; 3; 6", "unique map")
assert(vq{1,2,3,2,2,4}:unique():s() == "?; 1; 2; 3; 4", "unique rows")
assert(vq{1,2,3}:exceptmap(vq{2,4,5}):s() == "?; 1; 3", "except map")
assert(vq{1,2,3}:except(vq{2,4,5}):s() == "?; 1; 3", "except rows")
assert(vq{1,2,3}:isectmap(vq{2,1,5}):s() == "?; 1; 2", "intersect map")
assert(vq{1,2,3}:intersect(vq{2,1,5}):s() == "?; 1; 2", "intersect rows")
assert(vq{1,2,3}:union(vq{2,1,5}):s() == "?; 1; 2; 3; 5", "union rows")

-- relational projection
assert((vq{1,2,3,4,5}..vq{9,8,7,8,9}):project(2):s() == "?; 9; 8; 7", "project")

-- convert to table
local t = vq{1,2,3}:table()
assert(type(t) == "table", "t type")
assert(#t == 3, "table size")
assert(vq(t):s() == "?; 1; 2; 3", "table result as view")
local v2 = vq{meta='X,Y';'A','D','B','E','C','F'}
local t2 = v2:table()
assert(type(t2) == "table", "t2 type")
assert(t2.A == 'D' and t2.B == 'E' and t2.C == 'F', "t2 contents")

-- wheremap and where
assert(vq{1,2,3,2,1}:wheremap(function (r) return r[1]>=2 end):s() ==
  "?; 2; 3; 4", "simple wheremap")
assert(vq{1,2,3,2,1}:where(function (r) return r[1]>=2 end):s() ==
  "?; 2; 3; 2", "simple where")

-- selectmap, select, and find
assert(v2:selectmap'B':s() == "?; 2", "simple selectmap")
assert(v2:select'B':s() == "X Y; B E", "simple select")
assert(v2'B':s() == "X Y; B E", "simple select via V() notation")
assert(v2:selectmap{Y='E'}:s() == "?; 2", "simple selectmap with table")
assert(v2:select{Y='E'}:s() == "X Y; B E", "simple select with table")
assert(v2{Y='E'}:s() == "X Y; B E", "simple select via V() notation with table")
assert(v2:find'B'.Y == 'E', "simple find")
assert(v2:find{Y='E'}.X == 'B', "simple find with table")

-- rename
assert((v2%{X='Z1',[2]='Z2'}):s() == "Z1 Z2; A D; B E; C F", "rename")
assert((v2%{'Z1','Z2'}):s() == "Z1 Z2; A D; B E; C F", "rename list")
assert((v2%{[-2]='Z1',[-1]='Z2'}):s() == "Z1 Z2; A D; B E; C F", "rename neg")
assert((v2%'Z'):s() == "X Z; A D; B E; C F", "rename last column")

-- omit and omitmap
assert(vq(5):omitmap{2,4}:s() == "?; 1; 3; 5", "omitmap")
assert(vq(5):omitmap{4,2}:s() == "?; 1; 3; 5", "omitmap unsorted")
assert(vq(5):omitmap{4,2,4}:s() == "?; 1; 3; 5", "omitmap unsorted+dups")

-- replace
assert(vq{1,2,3}:replace(2,1,{9}):s() == "?; 1; 9; 3", "replace same size")
assert(vq{1,2}:replace(2,0,{9,8}):s() == "?; 1; 9; 8; 2", "replace insert")
assert(vq{1,2}:replace(3,0,{9,8}):s() == "?; 1; 2; 9; 8", "replace append")
assert(vq{1,2,3}:replace(2):s() == "?; 1; 3", "replace delete")
assert(vq{1,2,3}:replace(2,1,{9,8}):s() == "?; 1; 9; 8; 3", "replace with more")
assert(vq{1,2,3,4}:replace(2,2,{9}):s() == "?; 1; 9; 4", "replace with less")

-- append
assert(v2:append(1,2):s() == "X Y; A D; B E; C F; 1 2", "append one row")
assert(v2:append(3,4,'a','b'):s() == "X Y; A D; B E; C F; 1 2; 3 4; a b",
  "append two rows")

-- end-relative indexing
assert(v2[-1].X == 'a', "end-relative row access")
assert(v2[-1][-1] == 'b', "end-relative column access")

-- blocked views
local bv = vq{meta='_B[X:I]';vq{1,2,3},vq{4,5,6},vq{7,8}}
assert(bv[1]._B:s() == "?; 1; 2; 3", "blocked subview 1")
assert(bv[2]._B:s() == "?; 4; 5; 6", "blocked subview 2")
assert(bv[-1]._B:s() == "?; 7; 8", "blocked last subview")
assert(bv:blocked():s() == "X; 1; 2; 3; 7; 4; 5; 6; 8", "blocked view")

-- extended table to view conversion
assert(vq({a=1,b=2,c=3},"X"):sort():s() == "X; a; b; c", "table keys as view")
assert(vq({a=1,b=2,c=3},"X,Y"):sort():s() == "X Y; a 1; b 2; c 3",
  "table pairs as view")

-- open starkit
local db = vq.open('../etc/l.kit')
assert(#db == 1, "sk 1 root row")
assert(db:meta()[1].subv:dump() == [[
  name    type  subv
  ------  ----  ----
  name       6    #0
  parent     1    #0
  files      8    #4]], "sk structure")
assert(db[1].dirs:dump() == [[
  name       parent  files
  ---------  ------  -----
  <root>         -1     #2
  lib             0     #0
  app-listk       1     #3
  cgi1.6          1     #2]], "sk dirs")
assert(db[1].dirs[1].files:dump() == [[
  name       size  date        contents
  ---------  ----  ----------  --------
  main.tcl     67  1039266154       54b
  ChangeLog   167  1046092593      130b]], "sk dir 1")
assert(db[1].dirs[2].files:dump() == [[
  name  size  date  contents
  ----  ----  ----  --------]], "sk dir 2")
assert(db[1].dirs[3].files:dump() == [[
  name          size  date        contents
  ------------  ----  ----------  --------
  pkgIndex.tcl    72  1039266186       72b
  listk.tcl     2452  1046093102     1132b
  tcl2html.tcl  5801  1046093789     2434b]], "sk dir 3")
assert(db[1].dirs[4].files:dump() == [[
  name          size   date        contents
  ------------  -----  ----------  --------
  pkgIndex.tcl    532  1020940611      317b
  cgi.tcl       65885  1020940611    15621b]], "sk dir 4")
assert(#db[1].dirs[4].files[2].contents == 15621, "memo bytes access")

-- describe
assert(db:describe() ==
        "dirs[name:S,parent:I,files[name:S,size:I,date:I,contents:B]]",
        "sk description")
assert(db:meta():describe() ==
        "dirs[name:S,parent:I,files[name:S,size:I,date:I,contents:B]]",
        "sk meta desc")
assert(db:meta():meta():describe() == "name:S,type:I,subv:V", "sk meta meta")

-- sort bug
local sb = vq{meta='key';
  'Version','Description','Copyright','Release','System','Memory','Packages',
}
assert(sb:s() ==
  "key; Version; Description; Copyright; Release; System; Memory; Packages",
  "sort input")
assert(sb:sort():s() ==
  "key; Copyright; Description; Memory; Packages; Release; System; Version",
  "sort output")

-- type conversion
assert(vq.s2t('S') == vq.S, "string to type")
assert(vq.t2s(vq.S) == 'S', "type to string")

-- ungroup
local ug = db[1].dirs/'files'
local um = ug:ungroupmap()
-- strange but correct values when entries are < 0 due to +1 adjustment for :P
assert(um:s() == "?; 1; 0; 3; 0; -1; 4; 0", "ungroupmap")
assert(db[1].dirs[um]:dump() == [[
  name       parent  files
  ---------  ------  -----
  <root>         -1     #2
  <root>         -1     #2
  app-listk       1     #3
  app-listk       1     #3
  app-listk       1     #3
  cgi1.6          1     #2
  cgi1.6          1     #2]], "ungroup remap")
assert(#ug:ungrouper(um) == 7, "ungrouper rows")
assert(ug:ungrouper(um):meta():dump() == [[
  name      type  subv
  --------  ----  ----
  name         6    #0
  size         1    #0
  date         1    #0
  contents     7    #0]], "ungrouper meta")
assert(ug:ungrouper(um):dump() == [[
  name          size   date        contents
  ------------  -----  ----------  --------
  main.tcl         67  1039266154       54b
  ChangeLog       167  1046092593      130b
  pkgIndex.tcl     72  1039266186       72b
  listk.tcl      2452  1046093102     1132b
  tcl2html.tcl   5801  1046093789     2434b
  pkgIndex.tcl    532  1020940611      317b
  cgi.tcl       65885  1020940611    15621b]], "ungrouper")
assert(db[1].dirs:ungroup('files'):dump() == [[
  name       parent  name          size   date        contents
  ---------  ------  ------------  -----  ----------  --------
  <root>         -1  main.tcl         67  1039266154       54b
  <root>         -1  ChangeLog       167  1046092593      130b
  app-listk       1  pkgIndex.tcl     72  1039266186       72b
  app-listk       1  listk.tcl      2452  1046093102     1132b
  app-listk       1  tcl2html.tcl   5801  1046093789     2434b
  cgi1.6          1  pkgIndex.tcl    532  1020940611      317b
  cgi1.6          1  cgi.tcl       65885  1020940611    15621b]], "ungroup")

-- grouping
local g1 = vq{meta='A';'a','b','c','d','e','f','g'}
local gv = g1:grouped({2,4,8},{7,6,5,4,3,2,1})
assert(gv[1][1]:s() == 'A; g', "subgroup 1")
assert(gv[2][1]:s() == 'A; f; e', "subgroup 2")
assert(gv[3][1]:s() == 'A; d; c; b; a', "subgroup 3")
local g2 = vq{meta='A,B';'a',1,'b',2,'b',3,'c',2,'b','1','c',1}
assert(g2:s() == "A B; a 1; b 2; b 3; c 2; b 1; c 1", "group input")
local gi1, gi2, gi3 = g2:colmap('A'):groupinfo()
assert(gi1:s() == "?; 1; 2; 4", "groupinfo 1")
assert(gi2:s() == "?; 2; 5; 7", "groupinfo 2")
assert(gi3:s() == "?; 1; 2; 3; 5; 4; 6", "groupinfo 3")
local gr = g2:group{1}
assert(gr:meta():dump() == [[
  name  type  subv
  ----  ----  ----
  A        6    #0
           8    #1]], "group top-level meta")
assert(gr:meta()[2].subv:dump() == [[
  name  type  subv
  ----  ----  ----
  B        6    #0]], "group sub-level meta")
assert(gr:dump() == [[
  A  ? 
  -  --
  a  #1
  b  #3
  c  #2]], "group top result")
assert(gr[1][2]:s() == 'B; 1', "group 1 result")
assert(gr[2][2]:s() == 'B; 2; 3; 1', "group 2 result")
assert(gr[3][2]:s() == 'B; 2; 1', "group 3 result")

-- join and ijoin
local jl = vq{meta='A,B';'a',1,'b',2,'b',3,'c',2,'b','1','c',1}
assert(jl:s() == "A B; a 1; b 2; b 3; c 2; b 1; c 1", "join left input")
local jr = vq{meta='B,C';1,'x',3,'x',1,'y',4,'z'}
assert(jr:s() == "B C; 1 x; 3 x; 1 y; 4 z", "join right input")
local jo = jl:join(jr)
assert(jo:dump() == [[
  A  B  ? 
  -  -  --
  a  1  #2
  b  2  #0
  b  3  #1
  c  2  #0
  b  1  #2
  c  1  #2]], "join top result")
assert(jo[1][-1]:s() == 'C; x; y', "join 1 result")
assert(jo[2][-1]:s() == 'C', "join 2 result")
assert(jo[3][-1]:s() == 'C; x', "join 3 result")
assert(jo[4][-1]:s() == 'C', "join 4 result")
assert(jo[5][-1]:s() == 'C; x; y', "join 5 result")
assert(jo[6][-1]:s() == 'C; x; y', "join 6 result")
assert(jl:ijoin(jr):dump() == [[
  A  B  C
  -  -  -
  a  1  x
  a  1  y
  b  3  x
  b  1  x
  b  1  y
  c  1  x
  c  1  y]], "ijoin result")
  
print "End tops"
