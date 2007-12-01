#!/usr/local/bin/lua5.1

require "lvq"
print(lvq._VERSION)
print(lvq._DESCRIPTION)

m=lvq.new(nil, 0)
print(m)
print(lvq.new(m, 123))
print(lvq.new(nil, 123))
print(11)
--print(m:meta())
print(22)
print(#m)
print(33)
print(m[55])
print(m[55](66))
print(m[55].abc)
