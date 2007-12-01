#!/usr/local/bin/lua5.1

require "lvq"
print(lvq._VERSION)

m=lvq.new(nil, 0)
print(m)
print(lvq.new(m, 123))
print(lvq.new(nil, 123))

print(m:meta())
print(m:size())