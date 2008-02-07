#!/usr/bin/env lux

-- Columnwise data manipulation
-- 21/02/2001 jcw@equi4.com

-- This code manages all data in Lua tables for now.
--  
-- Views are created using:
--   view = View {'name1','name2',...}
--  
-- Views can be indexed (1-based), producing a row reference:
--   row = view[123]
--  
-- Info functions:
--   numcols = view:cols()	-- width of this view
--   numrows = view:rows()	-- height of this view
--   row = view:prow()		-- parent row, or nil
--   strlist = view:syms()	-- list of column names
--   colnum = view:lookup(sym)	-- locate col by name
--  
-- Data access functions:
--   value = view:item(rownum,colnum)
--   rowvec = view:getrow(rownum)
--  
-- Data modification functions:
--   view:setrow(rownum,rowvec)
--   view:insert(rownum,value1,value2,...)
--   newpos = view:append(value1,value2,...)
--   view:delete(rownum)
--  
-- Utility functions:
--   view:setprow(row)
--   view:dump(?title?)
--  
-- Rows have attributes which can be fetched and stored:
--   value = row.attr
--   value = row[colnum]
--   row.attr = value
--   row[colnum] = value
--  
-- Read-only custom views are created using:
--   view = CustomView {
--     cols = function (v) ... end,      -- returns width, int
--     syms = function (v) ... end,      -- returns string list
--     rows = function (v) ... end,      -- returns height, int
--     item = function (v,r,c) ... end,  -- returns one data item
--   }
--  
-- Pre-defined custom views:
--   result = view.sort()        -- sort on all cols, ascending
--   result = view.sort('name')  -- sort 'name' field, ascending
--   result = view.sort(+2,-1)   -- sort col 2 asc and col 1 desc
--   result = view.filter(f)     -- keep rows for which f is true
--   result = view.project(a)    -- projection, arg lists cols
--   result = view.rename(m)     -- renames fields to new ones
--   result = view.explode(v)    -- builds the cartesian product
--   result = view.compute(a)    -- compute from name/fun pairs
--  
-- Note: only the "cols" and "syms" handlers are called during
-- construction.  There is no data access (i.e. calls to "rows"
-- and "item" handlers are only performed while using results).

rowtag,viewtag,custviewtag=newtag(),newtag(),newtag()

viewops={}
-- return number of columns
function viewops:cols()
  return self.d[0]
end
-- return a list of column names
function viewops:syms()
  local r={}
  for c=1,self.d[0] do r[c]=self.d[c] end
  return r
end
-- return parent row (default is nil)
function viewops:prow()
  return rawget(self,'p')
end
-- return number of rows
function viewops:rows()
  return self.n
end
-- return specified attribute
function viewops:item(r,c)
  return self.c[c][r]
end
-- return position of a column name
function viewops:lookup(s)
  local t=self:syms()
  for i=1,getn(t) do if s==t[i] then return i end end
end
-- return specified row
function viewops:getrow(x)
  local r={}
  for c=1,self:cols() do r[c]=self:item(x,c) end
  return r
end
-- set specified row to new values
function viewops:setrow(x,r)
  for c=1,self:cols() do self.c[c][x]=r[c] end
end
-- insert args as row at specified position
function viewops:insert(pos,...)
  self.n=self.n+1
  for c=1,self:cols() do tinsert(self.c[c],pos,arg[c]) end
end
-- append args as row at end
function viewops:append(...)
  self.n=self.n+1
  for c=1,self:cols() do self.c[c][self.n]=arg[c] end
  return self.n
end
-- delete specified row
function viewops:delete(pos)
  for c=1,self:cols() do tremoved(self.c[c],pos) end
  self.n=self.n-1
end
-- set the parent row (or clear it)
function viewops:setprow(r)
  self.p=r
end
-- dump view structure and contents
function viewops:dump(title)
  if title then write(strrep('- ',25),'\n ',title,' ') end
  hdump(self:syms())
  for i=1,self:rows() do
    write(format(' %4d:\t',i))
    hdump(self:getrow(i))
  end
end
-- tag method to make views indexable
settagmethod(viewtag,'index',
  function (t,x)
    if type(x)=='number' then return settag({v=t,r=x},rowtag) end
    assert(viewops[x],x)
    return viewops[x]
  end)
-- tag method to fetch row attributes
settagmethod(rowtag,'gettable',
  function (t,c)
    local v,r=rawget(t,'v'),rawget(t,'r')
    assert(v and r)
    assert(1<=r and r<=v:rows(),r)
    if type(c)~='number' then c=v:lookup(c) end
    if c then return v:item(r,c) end
  end)
-- tag method to store row attributes
settagmethod(rowtag,'settable',
  function (t,x,z)
    local v,r=rawget(t,'v'),rawget(t,'r')
    assert(v and r)
    assert(1<=r and r<=v:rows(),r)
    local c=v:lookup(x)
    if c then v.c[c][r]=z end
  end)

-- construct a new view with specified fields
function View(d)
  local c={}
  d[0]=getn(d)
  for i=1,d[0] do d[d[i]]=i; c[i]={} end
  return settag({n=0,d=d,c=c},viewtag)
end

-- tag method to make custom views indexable
settagmethod(custviewtag,'index',
  function (t,x)
    if type(x)=='number' then return settag({v=t,r=x},rowtag) end
    local h=rawget(t,'h')
    if x=='n' then return h.rows(t) end
    return h[x] or function (...) return %h.dispatch(%x,arg) end
  end)
-- construct a new custom view with the specified handlers
function CustomView(h)
  if not h.dispatch then
    h.dispatch= function (x,args)
		  assert(viewops[x],x)
    		  return call(viewops[x],args)
		end
  end
  return settag({h=h},custviewtag)
end

function intrun(n)
  local r={}
  for i=1,n do r[i]=i end
  return r
end

function mysort(v)
  local b,o,m=v.b,v.o,intrun(v:rows())
  sort(m,
    function (x,y)
      for i=1,getn(%o) do
	local c=%o[i]
	if c<0 then
	  c=-c
	  if %b:item(x,c)>%b:item(y,c) then return 1 end
	else
	  if %b:item(x,c)<%b:item(y,c) then return 1 end
	end
        if %b:item(x,c)~=%b:item(y,c) then return end
      end
    end)
  v.m=m
  return m
end

sorting={
  cols = function (v) return v.b:cols() end,
  syms = function (v) return v.b:syms() end,
  rows = function (v) return v.b:rows() end,
  item = function (v,r,c)
	   local m=rawget(v,'m') or mysort(v)
	   return v.b:item(m[r],c)
	 end,
}

function viewops:sort(...)
  local r=CustomView(sorting)
  for i=1,getn(arg) do
    if type(arg[i])~='number' then arg[i]=self:lookup(arg[i]) end
  end
  if getn(arg)==0 then arg=intrun(self:cols()) end
  r.b,r.o=self,arg
  return r
end

function myfilter(v)
  local b,f,m=v.b,v.f,{}
  for i=1,b:rows() do if f(b,i) then tinsert(m,i) end end
  v.m=m
  return m
end

filtering={
  cols = function (v) return v.b:cols() end,
  syms = function (v) return v.b:syms() end,
  rows = function (v) return getn(rawget(v,'m') or myfilter(v)) end,
  item = function (v,r,c)
	   local m=rawget(v,'m') or myfilter(v)
	   return v.b:item(m[r],c)
	 end,
}

function viewops:filter(f)
  local r=CustomView(filtering)
  r.b,r.f=self,f
  return r
end

projecting={
  cols = function (v) return getn(v.m) end,
  syms = function (v)
	   local s,l=v.b:syms(),{}
	   for i=1,getn(v.m) do l[i]=s[v.m[i]] end
           return l
	 end,
  rows = function (v) return v.b:rows() end,
  item = function (v,r,c) return v.b:item(r,v.m[c]) end,
}

function viewops:project(a)
  local r,b=CustomView(projecting),{}
  for i=1,getn(a) do
    if type(a[i])=='number' then b[i]=a[i] else b[i]=self:lookup(a[i]) end
  end
  r.b,r.m=self,b
  return r
end

renaming={
  cols = function (v) return v.b:cols() end,
  syms = function (v)
	   local s=v.b:syms()
	   for i=1,getn(s) do s[i]=v.m[s[i]] or s[i] end
           return s
	 end,
  rows = function (v) return v.b:rows() end,
  item = function (v,r,c) return v.b:item(r,c) end,
}

function viewops:rename(m)
  local r=CustomView(renaming)
  r.b,r.m=self,m
  return r
end

exploding={
  cols = function (v) return v.b:cols()+v.m:cols() end,
  syms = function (v)
	   local s,t=v.b:syms(),v.m:syms()
	   for i=1,getn(t) do tinsert(s,t[i]) end
           return s
	 end,
  rows = function (v) return v.b:rows()*v.m:rows() end,
  item = function (v,r,c)
  	   local k,n=v.b:cols(),v.m:rows()
	   if c<=k then
	     return v.b:item(bor((r-1)/n,0)+1,c) -- tricky int()
	   end
	   return v.m:item(imod((r-1),n)+1,c-k)
	 end,
}

function viewops:explode(v)
  local r=CustomView(exploding)
  r.b,r.m=self,v
  return r
end

computing={
  cols = function (v) return v.b:cols()+getn(v.m)/2 end,
  syms = function (v)
	   local s,n=v.b:syms(),v.b:cols()
	   for i=1,getn(v.m),2 do n=n+1; s[n]=v.m[i] end
           return s
	 end,
  rows = function (v) return v.b:rows() end,
  item = function (v,r,c)
  	   local k=v.b:cols()
	   if c<=k then return v.b:item(r,c) end
	   return v.m[2*(c-k)](v,r)
	 end,
}

function viewops:compute(map)
  local r=CustomView(computing)
  r.b,r.m=self,map
  return r
end

-- horizontal dump
function hdump(t)
  local s='('
  for i=1,getn(t) do
    local v=t[i]
    if type(v)=='table' then v='<'..v:rows()..' rows>' end
    write(s,v)
    s=','
  end
  write(')\n')
end

if not omitDataInit then
  -- sample data from Gadfly by Aaron Watters

  frequents=View {'drinker','bar','perweek'}
  frequents:append('adam','lolas',1)
  frequents:append('woody','cheers',5)
  frequents:append('sam','cheers',5)
  frequents:append('norm','cheers',3)
  frequents:append('wilt','joes',2)
  frequents:append('norm','joes',1)
  frequents:append('lola','lolas',6)
  frequents:append('norm','lolas',2)
  frequents:append('woody','lolas',1)
  frequents:append('pierre','frankies',0)

  serves=View {'bar','beer','quantity'}
  serves:append('cheers','bud',500)
  serves:append('cheers','samaddams',255)
  serves:append('joes','bud',217)
  serves:append('joes','samaddams',13)
  serves:append('joes','mickies',2222)
  serves:append('lolas','mickies',1515)
  serves:append('lolas','pabst',333)
  serves:append('winkos','rollingrock',432)
  serves:append('frankies','snafu',5)

  likes=View {'drinker','beer','perday'}
  likes:append('adam','bud',2)
  likes:append('wilt','rollingrock',1)
  likes:append('sam','bud',2)
  likes:append('norm','rollingrock',3)
  likes:append('norm','bud',2)
  likes:append('nan','sierranevada',1)
  likes:append('woody','pabst',2)
  likes:append('lola','mickies',5)

  root=View {'frequents','serves','likes'}
  root:append(frequents,serves,likes)
  
  assert(likes[3].drinker=='sam')
  likes[3].drinker='john'
  assert(root[1].likes[3].drinker=='john')
  likes[3].drinker='sam'

  cvwdata={
    {'bill',55},
    {'john',44},
    {'walt',33},
  }
  cvw=CustomView {
    cols = function (v) return 2 end,
    syms = function (v) return {'name','age'} end,
    rows = function (v) return getn(cvwdata) end,
    item = function (v,r,c) return cvwdata[r][c] end,
  }

  assert(cvw:lookup('age')==2)
  assert(cvw[2].age==44)
end

if not omitDataDump then
  print('tags: row='..rowtag..', view='..viewtag ..', custview='..custviewtag)

  frequents:dump('frequents')
  serves:dump('serves')
  likes:dump('likes')
  root:dump('root')
  cvw:dump('cvw')

  frequents:sort('bar'):dump('sort')
  frequents:filter(function (v,r) return v:item(r,3)==2 end):dump('filter')
  frequents:project{'perweek','drinker'}:dump('project')
  frequents:rename{perweek='w',drinker='d'}:dump('rename')
  cvw:explode(likes):dump('explode')
  cvw:compute{'x',function (v,r) return 2*v[r].age end}:dump('compute')
end
