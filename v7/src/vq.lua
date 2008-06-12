--[[  Wrapper code for the C module.  
      $Id$
      This file is part of Vlerq, see src/vlerq.h for full copyright notice.
--]]

require "vq.core"

-- set up a global vops that can be used to define type-coerced view operators
local mt = getmetatable(vq())
vops = setmetatable({}, {
  __index = mt,
  __newindex = function (t,k,v)
    local sep = k:find('_',3) -- don't match two leading underscores
    if sep then
      vq.def(k:sub(1,sep-1), k:sub(sep+1), v)
    else
      mt[k] = v
    end
  end,
})

module ('vq', package.seeall)

function vq.open (fn)
  return vq.load(vq.map(fn))
end

local render = setmetatable({ [vq.N] = function (x) return '' end,
                              [vq.B] = function (x) return #x..'b' end,
                              [vq.V] = function (x) return '#'..#x end },
                            { __index = function (x) return tostring end })
function vops.dump (vw,maxrows)
  maxrows = math.min(maxrows or 20, #vw)
  -- set up column information
  local desc, funs, names, widths, meta = '', {}, {}, {}, vw:meta()
  if #meta == 0 then return '  ('..#vw..' rows, 0 columns)' end
  for c = 1,#meta do
    local t = meta[c].type
    desc = desc..'  %%'..((t == vq.S and '-') or '+')..'%ds'
    funs[c] = render[t]
    names[c] = meta[c].name
    if names[c] == '' then names[c] = '?' end
    widths[c] = #names[c]
  end
  -- collect all data and calculate maximum column widths
  local data = {}
  for r = 1,maxrows do
    data[r] = {}
    for c,f in ipairs(funs) do
      local x = vw[r][c]
      x = (x and f(x)) or '' -- show nil as empty string
      data[r][c] = x
      widths[c] = math.min(math.max(widths[c], #x), 50)
    end
  end
  -- set up formats and separators
  local seps, fmt = {}, desc:format(unpack(widths))
  for i,w in ipairs(widths) do
    seps[i] = string.rep('-', w)
  end
  local dashes = fmt:format(unpack(seps))
  -- collect all output
  local out = { fmt:gsub('+','-'):format(unpack(names)), dashes }
  for r,row in ipairs(data) do
    table.insert(out, fmt:format(unpack(row)))
  end
  if #vw > #data then
    table.insert(out, (dashes:gsub('-', '.')))
  end
  return table.concat(out, '\n')
end

function vops.find (v,...)
  local m = v:selectmap(...)
  if #m == 1 then return v[m[1][1]] end
end

local esc = { ['<']='&lt;', ['>']='&gt;', ["&"]='&amp;' }
local function htmlize (s)
  return s:gsub('[<>&]', esc)
end
function vops.html (v)
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
      if t == vq.B then z = #val..'b'
        elseif t == vq.V then z = val:html() else
          z = htmlize(tostring(val))
      end
      local a = ((t == vq.S or t == vq.V) and "") or " align='right'"
      add("<td%s>%s</td>", a, z)
    end
    add "</tr>"
  end
  add "</table>"
  return table.concat(o)
end

function vops.omitcol (v,col)
  return v/vq(v:width()):omitmap{col}
end

function vops.ungroup (v,col)
  local ncols = v:width()
  -- handle col by name and end-relative cols
  if type(col) == 'string' then 
    col = #v:meta():find(col)
  elseif col < 0 then
    col = col+ncols+1
  end
  -- expand and combine all but subview col with flattened ones
  local sub = v/col
  local map = sub:ungroupmap()
  return v:omitcol(col)[map] .. sub:ungrouper(map)
end

function vops.group (v, cols)
  local vkey = v/cols
  local vres = v/v:meta():omitmap(cols)
  local g1, g2, g3 = vkey:groupinfo()
  return vkey[g1] .. vres:grouped(g2, g3)
end

function vops.join (left, right)
  local lmeta = left:meta();
  local rmeta = right:meta();
  local lmap = lmeta:isectmap(rmeta)
  local rmap = lmeta:revisect(rmeta)
  local lkey = left/lmap
  local rkey = right/rmap
  local rres = right/right:meta():omitmap(rmap)
  local j1, j2, j3 = lkey:joininfo(rkey)
  return left .. rres:grouped(j2, j3)[j1]
end

function vops.save (v,fn)
  local s = v:emit()
  local f = io.open(fn,'wb')
  f:write(s)
  f:close()
  return #s
end

function vops.rows (v)
  local r = 0
  return function ()
    r = r+1
    if r <= #v then return v[r] end
  end
end

-- alternative implementation, not sure it is better:
--local function irows (v,r)
--  local n = #r+1
--  if n <= #v then return v[n] end
--end
--function vops.rows (v)
--  return irows, v, v[1]
--end

function vops.append (v,...) return v:replace(#v+1, 0, {meta=v:meta();...}) end
function vops.clone (v) return vq(0,v:meta()) end
function vops.except (v,w) return v[v:exceptmap(w)] end
function vops.first (v,n) return v:pair(n) end
function vops.ijoin (v,w) return v:join(w):ungroup(-1) end
function vops.intersect (v,w) return v[v:isectmap(w)] end
function vops.last (v,n) return v:reverse():pair(n):reverse() end
function vops.omit (v,w) return v[v:omitmap(w)] end
function vops.p (v,...) print(v:dump(...)) return v end
function vops.product_VV (v,w) return v:spread(w)..w:times(v) end
function vops.project (v,x) return (v/x):unique() end
function vops.reverse (v) return v[v:step(#v,-1)] end
function vops.select (v,...) return v[v:selectmap(...)] end
function vops.sort (v) return v[v:sortmap()] end
function vops.spread_VI (v,n) return v[vq(#v*n):step(1, 1, n)] end
function vops.times_VI (v,n) return v[vq(#v*n):step()] end
function vops.union (v,w) return v+vq(w):except(v) end
function vops.unique (v) return v[v:uniquemap()] end
function vops.where (v,f) return v[v:wheremap(f)] end   
function vops.width (v) return #v:meta() end

vops.__add = vops.plus
vops.__call = vops.select
vops.__concat = vops.pair
vops.__div = vops.colmap
vops.__mod = vops.rename
