#!/usr/bin/env lua

-- remove run-specific hex addresses from the output
function clean(s)
  return (tostring(s):gsub('0x%x+','0x[...]'))
end

require 'lvq'

assert(lvq._VERSION == 'LuaVlerq 1.6')
assert(lvq._CONFIG == 'lvq 1.6.0 di lo mu op ra sa ut')

m=vops.view(0)
assert(clean(m) == 'view: 0x[...] #0 ')
assert(clean(vops.view(123, m)) == 'view: 0x[...] #123 ')
assert(clean(vops.view(234)) == 'view: 0x[...] #234 ')

assert(clean(m.meta) == 'function: 0x[...]')
assert(clean(m:meta()) == 'view: 0x[...] #0 SI()')
assert(#m:meta() == 0)
assert(#m:meta():meta() == 3)
assert(#m:meta():meta():meta() == 3)

assert(#m == 0)
assert(clean(m[55]) == 'row: 0x[...] 55')

--print(m[55][66])
--print(m[55].abc)
--print(m[55]['abc'])
--m[1].abc=3
--print(m.blah)

--for k,v in pairs(vops) do print(k,v) end

function vops.blah (v)
  --print("v:", v, type(v))
  return #v:meta():meta()
end

assert(clean(m.blah) == 'function: 0x[...]')
assert(m:blah() == 3)

assert(m:dump() == '  (0 rows, 0 columns)')
assert(m:meta():dump() == [[
  name  type  subv
  ----  ----  ----]])
assert(m:meta():meta():dump() == [[
  name  type  subv
  ----  ----  ----
  name     5    #0
  type     1    #0
  subv     7    #0]])
assert(m:meta():meta():at(2,2):dump() == [[
  name  type  subv
  ----  ----  ----]])
assert(m:meta():meta():meta():dump() == [[
  name  type  subv
  ----  ----  ----
  name     5    #0
  type     1    #0
  subv     7    #0]])

v=vops.view(3,'first,age:I')
v[0].first, v[0].age = 'joe', 8
v[1].first, v[1].age = 'philip', 35
v[2].first, v[2].age = 'inez', 21
assert(v:dump() == [[
  first   age
  ------  ---
  joe       8
  philip   35
  inez     21]])

vv=vops.open('../etc/simple.db')
assert(clean(vv) == 'view: 0x[...] #1 (I)')
v0=vv[0].v
assert(v0:dump() == [[
  abc  
  -----
      1
     22
    333
   4444
  55555]])

v1=vops.open('../etc/alltypes.db')
assert(clean(v1) == 'view: 0x[...] #1 (IIIIIIILFDSB)')

assert(#v:emit() == 68)

--f=io.open('tmp1.db','wb')
--f:write(s)
--f:close()
--
--vops.open('tmp1.db'):p()
--os.remove('tmp1.db')

assert(v:save('tmp2.db') == 68)
vvv=vops.open('tmp2.db')
os.remove('tmp2.db')

mv=vvv:mutable()
mv:replace(1,1,0)
mv[1].first = 'shirley'
mv[0].age = nil
assert(mv:empty(0,1))
assert(type(mv[0].age) == 'nil')
assert(mv:dump() == [[
  first    age
  -------  ---
  joe         
  shirley   21]])
assert(vvv:type() == 'view')
assert(mv:type() == 'mutable')

assert(vops.iota(4,'N'):dump() == [[
  N
  -
  0
  1
  2
  3]])

assert(vops.virtual(4, "abc:I", function (r,c) return r*r*r end):dump() == [[
  abc
  ---
    0
    1
    8
   27]])

assert(vops.dump{meta='A:I,B,C:D'; 1,'two',3.3,4,'five',6.6,7,'eight',9.9} == [[
  A  B      C  
  -  -----  ---
  1  two    3.3
  4  five   6.6
  7  eight  9.9]])

assert(v0:rowmap({0,2,4,3,1}):dump() == [[
  abc  
  -----
      1
    333
  55555
   4444
     22]])

assert(v0:rowmap(8):last(4):dump() == [[
  abc  
  -----
  55555
      1
     22
    333]])

assert(vops.iota(3,'N'):rowmap(7):dump() == [[
  N
  -
  0
  1
  2
  0
  1
  2
  0]])

assert(vops.iota(3,'N'):reverse():rowmap(7):dump() == [[
  N
  -
  2
  0
  1
  2
  0
  1
  2]])

assert(v0:colmap(3):dump() == [[
  abc    abc    abc  
  -----  -----  -----
      1      1      1
     22     22     22
    333    333    333
   4444   4444   4444
  55555  55555  55555]])

assert(vops.plus(1,2,3,4):dump() == '  (10 rows, 0 columns)')

assert(v0:plus(v0):dump() == [[
  abc  
  -----
      1
     22
    333
   4444
  55555
      1
     22
    333
   4444
  55555]])

assert(v0:pair(v0,v0):dump() == [[
  abc    abc    abc  
  -----  -----  -----
      1      1      1
     22     22     22
    333    333    333
   4444   4444   4444
  55555  55555  55555]])

v1=v0..v0..v0
assert(v1:dump() == [[
  abc    abc    abc  
  -----  -----  -----
      1      1      1
     22     22     22
    333    333    333
   4444   4444   4444
  55555  55555  55555]])

v2=vops.open('../etc/lkit-le.db')
assert(clean(v2) == 'view: 0x[...] #1 (SI(SIIB))')

--v3=v2:at(0,0)
--print(v3:dump())
