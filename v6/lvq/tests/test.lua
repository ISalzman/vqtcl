#!/usr/local/bin/lua5.1

require 'lvq'
print(lvq._VERSION..' - '..lvq._CONFIG..'\n')

m=lvq.view(0)
print(m)
print(lvq.view(123, m))
print(lvq.view(234))
print(11)
print(m.meta)
print(m:meta())
print(#m:meta())
print(#m:meta():meta())
print(#m:meta():meta():meta())
print(22)
print(#m)
print(m[55])
--print(m[55][66])
--print(m[55].abc)
--print(m[55]['abc'])
print(33)
--m[1].abc=3
--print(m.blah)

mt=getmetatable(m)
--for k,v in pairs(mt) do print(k,v) end

function mt.blah (v)
  print("v:", v, type(v))
  return #v:meta():meta()
end

print(m.blah)
print(m:blah())

print(44)
m:p()
m:meta():p()
m:meta():meta():p()

print(55)
mm=m:meta():meta()

t=lvq.meta('first,age:I')
print(t)
t:p()

v=lvq.view(3,t)
v[1].first='joe'
v[1].age=8
v[2].first='philip'
v[2].age=35
v[3].first='inez'
v[3].age=21
print(v)
v:p()

lvq.open('../data/simple.db')[1].v:p()
lvq.open('../data/alltypes.db')[1].v:p()
