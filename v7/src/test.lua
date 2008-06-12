--[[  Main test file for LuaVlerq.
      $Id$
      This file is part of Vlerq, see src/vlerq.h for full copyright notice.
--]]

print(os.date())

-- with a -v cmdline argument, print each assertion instead of checking it
if arg and arg[1] == "-v" then assert = print end

require 'vq'
assert(vq._VERSION == "LuaVlerq 1.7", "version")

-- return view contents as a compact string
function vops.s (v)
  local m = v:meta()
  local x = {}
  for c = 1,#m do
    local z = m[c].name
    x[c] = (z == '' and '?') or z
  end
  local y = { table.concat(x, ' ') }
  for r = 1,#v do
    x = {}
    for c = 1,#m do
      local z = v[r][c] or ''
      x[c] = (type(z) == type(v) and '#'..#z) or tostring(z)
    end
    y[r+1] = table.concat(x, ' ')
  end
  return table.concat(y, '; ')
end

dofile "../src/tsmoke.lua"

if vops.calc then
	dofile "../src/tops.lua"
	dofile "../src/tmutable.lua"
	dofile "../src/trange.lua"
end

print "End test"
