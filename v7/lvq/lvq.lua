--[[  Wrapper module.  
      $Id$
      This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. ]]

require "lvq.core"

view = vops.view
vopdef = lvq.vopdef

module (..., package.seeall)

-- define some operators on views
getmetatable(view()).__add = vops.plus
getmetatable(view()).__concat = vops.pair

-- table with render functions for all data types, N=0 S=5 B=6 V=7
local renderers = { [0] = function (x) return '' end,
                    [6] = function (x) return #x..'b' end,
                    [7] = function (x) return '#'..#x end }
setmetatable(renderers, { __index = function (x) return tostring end })

-- produce a pretty-printed tabular string from a view
vopdef('dump', 'V', function (vw, maxrows)
  maxrows = math.min(maxrows or 20, #vw)
  -- set up column information
  local desc, funs, names, widths, meta = '', {}, {}, {}, vw:meta()
  if #meta == 0 then return '  ('..#vw..' rows, 0 columns)' end
  for c = 1,#meta do
    local t = meta[c-1].type
    desc = desc..'  %%'..(t == 5 and '-' or '+')..'%ds'
    funs[c] = renderers[t]
    names[c] = meta[c-1].name
    if names[c] == '' then names[c] = '?' end
--print(meta,c,names[c])
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

-- repeat all view rows n times
vopdef ('times', 'VI', function (v, n)
  return v [view(#v*n):step()]
end)

-- repeat each row of a view n times
vopdef ('spread', 'VI', function (v, n)
  return v [view(#v*n):step(0, 1, n)]
end)

-- cross product
vopdef ('product', 'VV', function (v, w)
  return v:spread(#w) .. w:times(#v)
end)

-- first n rows
vopdef ('first', 'VI', function (v, n)
  return n < #v and view(n)..v or v
end)

-- last n rows
vopdef ('last', 'VI', function (v, n)
  return n < #v and v [view(n):step(#v-n)] or v
end)

-- reverse row order
vopdef ('reverse', 'V', function (v)
  return v [v:step(#v-1, -1)]
end)
