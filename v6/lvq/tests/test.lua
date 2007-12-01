#!/usr/local/bin/lua5.1

require "lvq"
print(lvq._VERSION)
print(lvq._DESCRIPTION)

m=lvq.view(nil, 0)
print(m)
print(lvq.view(m, 123))
print(lvq.view(nil, 234))
print(11)
--print(m:meta())
print(22)
print(#m)
print(m[55])
print(m[55](66))
print(m[55].abc)
print(m[55]['abc'])
print(33)
--m[1].abc = 3
