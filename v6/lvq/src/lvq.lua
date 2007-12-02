--[[  Wrapper module.  
      This file is part of LuaVlerq.
      See lvq.h for full copyright notice.
      $Id$  ]]--

local lvq = require 'lvq.core'

module ('lvq', package.seeall)

assert(_VERSION == 'LuaVlerq 1.6')

local mt=getmetatable(lvq.view(0))

mt.step    = lvq.step    -- function (count, start, step, rate)
mt.vconcat = lvq.vconcat -- function (v1, v2)
mt._pair   = lvq._pair   -- function (meta, v1, v2)

function mt.__concat (a, b)
  return mt._pair(mt.vconcat(a:meta(), b:meta()), a, b)
end

function mt.hrepeat (v, n)
  if n==0 then 
    return lvq.view(#v)
  end
  local w={}
  for i=1,n do
    table.insert(w, v)
  end
  return mt.pair(unpack(w))
end

function mt.vrepeat (v, n)
  return v:step(n * #v, 0, 1, 1)
end

function mt.spread (v, n)
  return v:step(n * #v, 0, 1, n)
end

function mt.product (a, b)
  return a:spread(#b) .. b:vrepeat(#a)
end

-- table with render functions for each data type, N=0 S=5 B=6 V=7
local renderers = { [0] = function (x) return '' end,
                    [6] = function (x) return #x..'b' end,
                    [7] = function (x) return '#'..#x end }
setmetatable(renderers, { __index = function (x) return tostring end })

-- produce a pretty-printed string table from a view
function mt.dump (vw, maxrows)
  maxrows = math.min(maxrows or 20, #vw)
  -- set up column information
  local desc, funs, names, widths, meta = '', {}, {}, {}, vw:meta()
  if #meta == 0 then return '  ('..#vw..' rows, 0 columns)' end
  for c = 1,#meta do
    local t = meta[c].type
    desc = desc..'  %%'..(t == 5 and '-' or '')..'%ds'
    funs[c] = renderers[t]
    names[c] = meta[c].name
    widths[c] = #names[c]
  end
  -- collect all data and calculate maximum column widths
  local data = {}
  for r = 1,maxrows do
    data[r] = {}
    for c,f in ipairs(funs) do
      local x = f(vw[r][c])
      data[r][c] = x
      widths[c] = math.max(widths[c], #x)
    end
  end
  -- set up formats and separators
  local seps, fmt = {}, string.format(desc, unpack(widths))
  for i,w in ipairs(widths) do
    seps[i] = string.rep('-', w)
  end
  local dashes = string.format(fmt, unpack(seps))
  -- collect all output
  local out = {}
  table.insert(out, string.format(fmt, unpack(names)))
  table.insert(out, dashes)
  for _,row in ipairs(data) do
    table.insert(out, string.format(fmt, unpack(row)))
  end
  if #vw > #data then
    table.insert(out, (string.gsub(dashes, '-', '.')))
  end
  return table.concat(out, '\n')
end
