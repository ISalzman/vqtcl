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
print(m[55][66])
print(m[55].abc)
print(m[55]['abc'])
print(33)
--m[1].abc=3
--print(m.blah)

mt=getmetatable(m)
for k,v in pairs(mt) do print(k,v) end

function mt.blah (v)
  print("v:", v, type(v))
  return #v:meta():meta()
end

print(m.blah)
print(m:blah())

mm=m:meta():meta()
print(mm[1][1])
print(mm[1].name)
print(mm[2].name)
print(mm[3].name)
print(mm[1].type)
print(mm[2].type)
print(mm[3].type)
print(#mm[1].subv)
print(#mm[2].subv)
print(#mm[3].subv)
