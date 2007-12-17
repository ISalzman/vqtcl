--[[  Wrapper module.  
      $Id$
      This file is part of Vlerq, see base/vlerq.h for full copyright notice. ]]

require 'lvq.core'
module (..., package.seeall)
assert(_VERSION == 'LuaVlerq 1.6')

-- table with render functions for all data types, N=0 S=5 B=6 V=7
local renderers = { [0] = function (x) return '' end,
                    [6] = function (x) return #x..'b' end,
                    [7] = function (x) return '#'..#x end }
setmetatable(renderers, { __index = function (x) return tostring end })

-- produce a pretty-printed tabular string from a view
vopdef ('dump', 'V', function (vw, maxrows)
  maxrows = math.min(maxrows or 20, #vw)
  -- set up column information
  local desc, funs, names, widths, meta = '', {}, {}, {}, vw:meta()
  if #meta == 0 then return '  ('..#vw..' rows, 0 columns)' end
  for c = 1,#meta do
    local t = meta[c-1].type
    desc = desc..'  %%'..(t == 5 and '-' or '+')..'%ds'
    funs[c] = renderers[t]
    names[c] = meta[c-1].name
    widths[c] = #names[c]
  end
  -- collect all data and calculate maximum column widths
  local data = {}
  for r = 1,maxrows do
    data[r] = {}
    for c,f in ipairs(funs) do
      local x = vw[r-1][c-1]
      x = x and f(x) or '' -- show nil as empty string
      data[r][c] = x
      widths[c] = math.max(widths[c], #x)
    end
  end
  -- set up formats and separators
  local seps, fmt = {}, desc:format(unpack(widths))
  for i,w in ipairs(widths) do
    seps[i] = string.rep('-', w)
  end
  local dashes = fmt:format(unpack(seps))
  -- collect all output
  local out = {}
  out[1] = fmt:gsub('+','-'):format(unpack(names))
  out[2] = dashes
  for r,row in ipairs(data) do
    out[r+2] = fmt:format(unpack(row))
  end
  if #vw > #data then
    table.insert(out, (dashes:gsub('-', '.')))
  end
  return table.concat(out, '\n')
end)

-- shorthand for debugging: add ":p()" to print the view at that point
vopdef ('p', 'V', function (v, ...)
  print(v:dump(...))
  return v
end)

-- save a view to file in Metakit format
vopdef ('save', 'VS', function (v, fn) 
  local s = v:emit()
  local f = io.open(fn,'wb') -- created only if emit completes successfully
  f:write(s)
  f:close()
  return #s
end)

-- tentative definitions, not fully worked out yet

-- vops.step    = function (count, start, step, rate)
-- vops.vconcat = function (...)
-- vops._mcolcat   = function (meta, ...)

-- column catenate
vopdef ('colcat', '', function (...)
  local m = {}
  for i = 1, select('#', ...) do
    m[i] = select(i, ...):meta()
  end
  return vops._colcat(vops.vconcat(unpack(m)), ...)
end)

-- define the .. operator as binary version of colcat
vops.__concat = vops.colcat

-- repeat all columns n times
vopdef ('crep', 'VI', function (v, n)
  if n==0 then return #v end
  local w=v
  for i=2,n do w = w .. v end
  return w
end)

------------------------------------------ see http://www.equi4.com/ratcl/v6vops

-- return the size of a view as view
vopdef ('size', 'V', function (v)
  return v:colmap(0)
end)

-- return a 1-column view with 0..N-1 ints
vopdef ('iota', 'IS', function (v,name)
  --return v:step(0,1,1,name)
  return vops.step(v,0,1,1,name)
end)

-- return view with rows in reverse order
vopdef ('reverse', 'V', function (v)
  return v:rowmap(v:step(#v-1,-1))
end)

-- take n rows from either start or end of a view
vopdef ('take', 'VI', function (v,n)
  if n<0 then n, v = -n, v:reverse() end
  return v:rowmap(n)
end)

-- return the first n rows
vopdef ('first', 'VI', function (v,n)
  if n>#v then n=#v end
  return v:rowmap(n)
end)

-- return the last n rows
vopdef ('last', 'VI', function (v,n)
  if n>#v then n=#v end
  return v:rowmap(vops.step(n,#v-n))
end)

-- add a numbered tag column to a view
vopdef ('tag', 'VS', function (v,name)
  return v..v:iota(name)
end)

-- return specific rows from a view
vopdef ('slice', 'VVII', function (v,...)
  return v:rowmap(vops.step(...))
end)

-- repeat all rows n times
vopdef ('rrep', 'VI', function (v,n)
  return v:rowmap(n*#v)
end)

-- repeat each row n times
vopdef ('spread', 'VI', function (v,n)
  return v:rowmap(vops.step(n*#v,0,1,n))
end)

-- cross product
vopdef ('product', 'VV', function (v,w)
  return v:spread(w)..w:rrep(v)
end)

-- box an integer into a new 1-row/1-col view
vopdef ('intbox', 'I', function (i)
  return vops.step(1,i)
end)

-- create a view with same structure but no rows
vopdef ('clone' ,'V', function (v)
  return v:rowmap(0)
end)
