#!/usr/bin/env lua

require 'lvq'
print(lvq._VERSION..' - '..lvq._CONFIG..'\n')

m=vops.view(0)
print(m)
--print(vops.view(123, m))
--print(vops.view(234))
print(11)
print(m.meta)
--print(m:meta())
--print(#m:meta())
--print(#m:meta():meta())
--print(#m:meta():meta():meta())
print(22)
--print(#m)
print(m[55])
--print(m[55][66])
--print(m[55].abc)
--print(m[55]['abc'])
print(33)
--m[1].abc=3
--print(m.blah)

--for k,v in pairs(vops) do print(k,v) end

function vops.blah (v)
  --print("v:", v, type(v))
  return #v:meta():meta()
end

--print(m.blah)
print(m:blah())

print(44)
--m:p()
--m:meta():p()
m:meta():meta():p()

print(55)
v=vops.view(3,'first,age:I')
v[0].first, v[0].age = 'joe', 8
v[1].first, v[1].age = 'philip', 35
v[2].first, v[2].age = 'inez', 21
v:p()

vv=vops.open('../etc/simple.db')
print(vv)
v0=vv[0].v
v0:p()
print(vops.open('../etc/alltypes.db'))

--s=v:emit()
--print(#s)
--f=io.open('tmp1.db','wb')
--f:write(s)
--f:close()
--
--vops.open('tmp1.db'):p()
--os.remove('tmp1.db')

print(v:save('tmp2.db'))
vvv=vops.open('tmp2.db')
os.remove('tmp2.db')

mv=vvv:mutable()
mv:replace(1,1,0)
mv[1].first = 'shirley'
mv[0].age = nil
print(mv:empty(0,1))
print(type(mv[0].age))
mv:p()
print(vvv:type())
print(mv:type())

vops.iota(4,'N'):p()
--vops.iota(4,'N',1):p()
--vops.iota(4,'N',0):p()

vops.virtual(4, "abc:I", function (r,c) return r*r*r end):p()

vops.p{meta='A:I,B,C:D'; 1,'two',3.3,4,'five',6.6,7,'eight',9.9}

v0:rowmap({0,2,4,3,1}):p()
v0:rowmap(8):last(4):p()

--vops.iota(3,'N'):rowmap(7):p()
--vops.iota(3,'N'):reverse():rowmap(7):p()

v0:colmap(3):p()

--vops.rowcat(1,2,3,4):p()
--v0:rowcat(v0):p()
--v0:colcat(v0,v0):p()
--v1=v0..v0..v0; v1:p()
