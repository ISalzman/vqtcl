#! /usr/bin/env lua

require 'thrive'
w = thrive.new()
print(w)
w:load(' 0 exit exec -1 syst [[ ')
print(w:find('+'))
print(w:eval('54321'))
print(w:run())
print(w:pop())
w:push(123,456)
print(w:eval('+'))
print(w:run())
print(w:pop())
w:push(12,34)
print(w:run(w:find('+')))
print(w:pop())
w:push({1,nil,'a'})
x = w:pop()
for i,j in pairs(x) do
  print(i,j)
end
print(#x)

--[[ output:

thrive_ws (0x8070c68)
thrive_ref (1 @ 0x8070c68)
149
0
54321
33
0
579
-1
46
1       1
3       a
3

--]]
