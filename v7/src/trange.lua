--[[  Range primitive tests for LuaVlerq, called from test.lua wrapper.
      $Id$
      This file is part of Vlerq, see src/vlerq.h for full copyright notice.
--]]

-- convert string to position vec
local function s2p (s)
  local t = {}
  for x in s:gmatch("-?%d+") do table.insert(t, tonumber(x)+1) end
  return vq(t)
end

-- convert position vec to string
local function p2s (p)
  local t = p:table()
  for i=1,#t do t[i] = t[i]-1 end
  return table.concat(t, ' ')
end

-- assertion wrappers, using 0-based values for all inputs and results

local function t_flip (inp,off,cnt)
  return p2s(s2p(inp):r_flip(off+1,cnt))
end
local function t_locate (inp,off)
  local x, y = s2p(inp):r_locate(off+1)
  return (x-1)..'.'..(y-1)
end
local function t_insert (inp,off,cnt,mod)
  return p2s(s2p(inp):r_insert(off+1,cnt,mod))
end
local function t_delete (inp,off,cnt)
  return p2s(s2p(inp):r_delete(off+1,cnt))
end

assert(t_flip(''       ,1,0) == ''           , "t_flip 01")
assert(t_flip(''       ,1,1) == '1 2'        , "t_flip 02")
assert(t_flip(''       ,1,2) == '1 3'        , "t_flip 03")
assert(t_flip('1 3'    ,1,1) == '2 3'        , "t_flip 04")
assert(t_flip('1 3'    ,1,2) == ''           , "t_flip 05")
assert(t_flip('1 3'    ,3,2) == '1 5'        , "t_flip 06")
assert(t_flip('1 3'    ,5,3) == '1 3 5 8'    , "t_flip 07")
assert(t_flip('1 8'    ,3,2) == '1 3 5 8'    , "t_flip 08")
assert(t_flip('5 8'    ,1,2) == '1 3 5 8'    , "t_flip 09")
assert(t_flip('1 3 5 8',3,1) == '1 4 5 8'    , "t_flip 10")
assert(t_flip('1 3 5 8',4,1) == '1 3 4 8'    , "t_flip 11")
assert(t_flip('1 3 5 8',3,2) == '1 8'        , "t_flip 12")
assert(t_flip('1 3 5 8',1,2) == '5 8'        , "t_flip 13")
assert(t_flip('1 3 5 8',5,3) == '1 3'        , "t_flip 14")
assert(t_flip('1 3 5 8',0,1) == '0 3 5 8'    , "t_flip 15")
assert(t_flip('1 3 5 8',8,1) == '1 3 5 9'    , "t_flip 16")
assert(t_flip('1 3 5 8',6,1) == '1 3 5 6 7 8', "t_flip 17")
assert(t_flip('1 3 5 8',9,0) == '1 3 5 8'    , "t_flip 18")

assert(t_locate('',   3) == '-1.3', "t_locate 01")
assert(t_locate('1 2',1) == '0.0' , "t_locate 02")

local r = {}
for i=1,10 do
  table.insert(r, (i-1)..':')
  table.insert(r, t_locate('1 3 5 8', i))
end
--[[
print'0: -1.0 1: 0.0 2: 0.1 3: 1.1 4: 1.2 5: 2.2 6: 2.3 7: 2.4 8: 3.3 9: 3.4'
assert(table.concat(r, ' ') ==
  '0: -1.0 1: 0.0 2: 0.1 3: 1.1 4: 1.2 5: 2.2 6: 2.3 7: 2.4 8: 3.3 9: 3.4',
  "t_locate 03")
--]]

assert(t_insert(''       ,1,1,0) == ''           , "t_insert 01")
assert(t_insert(''       ,1,1,1) == '1 2'        , "t_insert 02")
assert(t_insert(''       ,1,2,0) == ''           , "t_insert 03")
assert(t_insert(''       ,1,2,1) == '1 3'        , "t_insert 04")
assert(t_insert(''       ,1,3,0) == ''           , "t_insert 05")
assert(t_insert(''       ,1,3,1) == '1 4'        , "t_insert 06")
--assert(t_insert('1 2'    ,1,1,1) == '1 2'        , "t_insert 07")

assert(t_insert('1 3'    ,0,2,0) == '3 5'        , "t_insert 11")
assert(t_insert('1 3'    ,0,2,1) == '0 2 3 5'    , "t_insert 12")
assert(t_insert('1 3'    ,1,2,0) == '3 5'        , "t_insert 13")
assert(t_insert('1 3'    ,1,2,1) == '1 5'        , "t_insert 14")
assert(t_insert('1 3'    ,2,2,0) == '1 2 4 5'    , "t_insert 15")
assert(t_insert('1 3'    ,2,2,1) == '1 5'        , "t_insert 16")
assert(t_insert('1 3'    ,3,2,0) == '1 3'        , "t_insert 17")
assert(t_insert('1 3'    ,3,2,1) == '1 5'        , "t_insert 18")
assert(t_insert('1 3'    ,4,2,0) == '1 3'        , "t_insert 19")
assert(t_insert('1 3'    ,4,2,1) == '1 3 4 6'    , "t_insert 20")

assert(t_insert('1 3 5 6',0,2,0) == '3 5 7 8'    , "t_insert 31")
assert(t_insert('1 3 5 6',0,2,1) == '0 2 3 5 7 8', "t_insert 32")
assert(t_insert('1 3 5 6',1,2,0) == '3 5 7 8'    , "t_insert 33")
assert(t_insert('1 3 5 6',1,2,1) == '1 5 7 8'    , "t_insert 34")
assert(t_insert('1 3 5 6',2,2,0) == '1 2 4 5 7 8', "t_insert 35")
assert(t_insert('1 3 5 6',2,2,1) == '1 5 7 8'    , "t_insert 36")
assert(t_insert('1 3 5 6',3,2,0) == '1 3 7 8'    , "t_insert 37")
assert(t_insert('1 3 5 6',3,2,1) == '1 5 7 8'    , "t_insert 38")
assert(t_insert('1 3 5 6',4,2,0) == '1 3 7 8'    , "t_insert 39")
assert(t_insert('1 3 5 6',4,2,1) == '1 3 4 6 7 8', "t_insert 40")
assert(t_insert('1 3 5 6',5,2,0) == '1 3 7 8'    , "t_insert 41")
assert(t_insert('1 3 5 6',5,2,1) == '1 3 5 8'    , "t_insert 42")
assert(t_insert('1 3 5 6',6,2,0) == '1 3 5 6'    , "t_insert 43")
assert(t_insert('1 3 5 6',6,2,1) == '1 3 5 8'    , "t_insert 44")
assert(t_insert('1 3 5 6',7,2,0) == '1 3 5 6'    , "t_insert 45")
assert(t_insert('1 3 5 6',7,2,1) == '1 3 5 6 7 9', "t_insert 46")

assert(t_delete(''       ,1,2) == ''       , "t_delete 01")
assert(t_delete('1 3'    ,0,1) == '0 2'    , "t_delete 02")
assert(t_delete('1 3'    ,1,1) == '1 2'    , "t_delete 03")
assert(t_delete('1 3'    ,2,1) == '1 2'    , "t_delete 04")
assert(t_delete('1 3'    ,3,1) == '1 3'    , "t_delete 05")

assert(t_delete('1 3 5 6',0,1) == '0 2 4 5', "t_delete 11")
assert(t_delete('1 3 5 6',1,1) == '1 2 4 5', "t_delete 12")
assert(t_delete('1 3 5 6',2,1) == '1 2 4 5', "t_delete 13")
assert(t_delete('1 3 5 6',3,1) == '1 3 4 5', "t_delete 14")
assert(t_delete('1 3 5 6',4,1) == '1 3 4 5', "t_delete 15")
assert(t_delete('1 3 5 6',5,1) == '1 3'    , "t_delete 16")
assert(t_delete('1 3 5 6',6,1) == '1 3 5 6', "t_delete 17")

assert(t_delete('1 3 5 6',0,2) == '0 1 3 4', "t_delete 21")
assert(t_delete('1 3 5 6',1,2) == '3 4'    , "t_delete 22")
assert(t_delete('1 3 5 6',2,2) == '1 2 3 4', "t_delete 23")
assert(t_delete('1 3 5 6',3,2) == '1 4'    , "t_delete 24")
assert(t_delete('1 3 5 6',4,2) == '1 3'    , "t_delete 25")
assert(t_delete('1 3 5 6',5,2) == '1 3'    , "t_delete 26")

assert(t_delete('1 3 5 6',0,3) == '2 3'    , "t_delete 31")
assert(t_delete('1 3 5 6',1,3) == '2 3'    , "t_delete 32")
assert(t_delete('1 3 5 6',2,3) == '1 3'    , "t_delete 33")
assert(t_delete('1 3 5 6',3,3) == '1 3'    , "t_delete 34")
assert(t_delete('1 3 5 6',4,3) == '1 3'    , "t_delete 35")
assert(t_delete('1 3 5 6',5,3) == '1 3'    , "t_delete 36")

assert(t_delete('1 3 5 6',0,4) == '1 2'    , "t_delete 41")
assert(t_delete('1 3 5 6',1,4) == '1 2'    , "t_delete 42")
assert(t_delete('1 3 5 6',2,4) == '1 2'    , "t_delete 43")
assert(t_delete('1 3 5 6',3,4) == '1 3'    , "t_delete 44")
assert(t_delete('1 3 5 6',4,4) == '1 3'    , "t_delete 45")
assert(t_delete('1 3 5 6',5,4) == '1 3'    , "t_delete 46")

assert(t_delete('1 3 5 6',0,5) == '0 1'    , "t_delete 51")
assert(t_delete('1 3 5 6',1,5) == ''       , "t_delete 52")
assert(t_delete('1 3 5 6',2,5) == '1 2'    , "t_delete 53")
assert(t_delete('1 3 5 6',3,5) == '1 3'    , "t_delete 54")
assert(t_delete('1 3 5 6',4,5) == '1 3'    , "t_delete 55")
assert(t_delete('1 3 5 6',5,5) == '1 3'    , "t_delete 56")

assert(t_delete('1 3 5 6',0,6) == ''       , "t_delete 61")
assert(t_delete('1 3 5 6',1,6) == ''       , "t_delete 62")
assert(t_delete('1 3 5 6',2,6) == '1 2'    , "t_delete 63")
assert(t_delete('1 3 5 6',3,6) == '1 3'    , "t_delete 64")
assert(t_delete('1 3 5 6',4,6) == '1 3'    , "t_delete 65")
assert(t_delete('1 3 5 6',5,6) == '1 3'    , "t_delete 66")
assert(t_delete('1 3 5 6',6,6) == '1 3 5 6', "t_delete 67")

assert(t_delete('0 1 3 5 7 8',0,2) == '1 3 5 6'    , "t_delete 71")
assert(t_delete('0 1 3 5 7 8',1,2) == '0 3 5 6'    , "t_delete 72")
assert(t_delete('0 1 3 5 7 8',2,2) == '0 1 2 3 5 6', "t_delete 73")
assert(t_delete('0 1 3 5 7 8',3,2) == '0 1 5 6'    , "t_delete 74")
assert(t_delete('0 1 3 5 7 8',2,4) == '0 1 3 4'    , "t_delete 75")
assert(t_delete('0 1 3 5 7 8',1,6) == '0 2'        , "t_delete 76")

assert(t_delete('0 2 3 5 6 8',0,2) == '1 3 4 6'    , "t_delete 81")
assert(t_delete('0 2 3 5 6 8',1,2) == '0 3 4 6'    , "t_delete 82")
assert(t_delete('0 2 3 5 6 8',2,2) == '0 3 4 6'    , "t_delete 83")
assert(t_delete('0 2 3 5 6 8',3,2) == '0 2 4 6'    , "t_delete 84")
assert(t_delete('0 2 3 5 6 8',2,4) == '0 4'        , "t_delete 85")
assert(t_delete('0 2 3 5 6 8',1,6) == '0 2'        , "t_delete 86")

assert(t_insert(''   ,2,5,1) == '2 7', "t_insert 99")
assert(t_delete('2 7',3,3  ) == '2 4', "t_delete 99")

print "End trange"
