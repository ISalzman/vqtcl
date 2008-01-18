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

-- html special char escaping, see "html" vopdef
local escaped = { ['<']='&lt;', ['>']='&gt;', ["&"]='&amp;' }
local function htmlize(text)
  return text:gsub('[<>&]', escaped)
end

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
    desc = desc..'  %%'..((t == 5 and '-') or '+')..'%ds'
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
      x = (x and f(x)) or '' -- show nil as empty string
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

-- set up row iteration, for use in a for loop
vopdef ('rows', 'V', function (v)
  local r = -1
  return function ()
    r = r+1
    if r < #v then return v[r] end
  end
end)

-- render view as (possibly nested) html table
vopdef ('html', 'V', function (v)
  local o = {[[<table>
    <style type="text/css"><!--
      tt table, pre table
        { border-spacing: 0; border: 1px solid #aaaaaa; margin: 0 0 2px 0 }
      th { background-color: #eeeeee; font-weight:normal }
      td { vertical-align: top }
      th, td { padding: 0 5px }
      th.row,td.row { color: #cccccc; font-size: 75% }
    --></style>
    <tr><th class="row"></th>]]}
  local function add(f, ...) table.insert(o, f:format(...)) end
  for r in v:meta():rows() do add("<th><i> %s </i></th>", r.name) end
  add "</tr>"
  for r in v:rows() do
    add("<tr><td align='right' class='row'>%d</td>", #r)
    for m in v:meta():rows() do
      local val, t, z = r[#m], m.type
      if t == 6 then z = #val..'b'
        elseif t == 7 then z = val:html() else
          z = htmlize(tostring(val))
      end
      local a = ((t == 5 or t == 7) and "") or " align='right'"
      add("<td%s>%s</td>", a, z)
    end
    add "</tr>"
  end
  add "</table>"
  return table.concat(o)
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
  return (n < #v and view(n)..v) or v
end)

-- last n rows
vopdef ('last', 'VI', function (v, n)
  return (n < #v and v [view(n):step(#v-n)]) or v
end)

-- reverse row order
vopdef ('reverse', 'V', function (v)
  return v [v:step(#v-1, -1)]
end)
