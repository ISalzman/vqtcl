--[[  Wrapper module.  
      This file is part of LuaVlerq.
      See lvq.h for full copyright notice.
      $Id$  ]]--

local lvq = require "lvq.core"

module ("lvq", package.seeall)

assert(_VERSION == "LuaVlerq 1.6.0")

local m={}

m.step    = lvq.step   -- function (count, start, step, rate)
m.concat  = lvq.concat -- function (...)
m.mpair   = lvq.mpair  -- function (meta, ...)

function m.pair (...)
  local w={}
  for i, v in ipairs(...) do
    table.insert(w, v:meta())
  end
  return m.mpair(m.concat(unpack(w)), ...)
end

function m.hrepeat (v, n)
  if n==0 then 
    return lvq.view(nil, #v)
  end
  local w={}
  for i=1,n do
    table.insert(w, v)
  end
  return m.pair(unpack(w))
end

function m.vrepeat (v, n)
  return v:step(n * #v, 0, 1, 1)
end

function m.spread (v, n)
  return v:step(n * #v, 0, 1, n)
end

function m.product(a, b)
  return a:spread(#b):pair(b:vrepeat(#a))
end
