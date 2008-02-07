#!/usr/bin/env lux

-- Grouping additions to qdata
-- 01/03/2001 jcw@equi4.com

omitDataDump=1
dofile('qdata.lua')

function commoncols(s,t)
  local a,b=s:syms(),t:syms()
  local m,x={},{}
  for i=1,getn(a) do m[a[i]]=i end
  for i=1,getn(b) do if m[b[i]] then tinsert(x,b[i]) end end
  if getn(x)~=s:cols() then s=s:project(x) end
  if getn(x)~=t:cols() then t=t:project(x) end
  return s,t
end

concatenating={
  cols = function (v) return v.b:cols() end,
  syms = function (v) return v.b:syms() end,
  rows = function (v) return v.b:rows()+v.m:rows() end,
  item = function (v,r,c)
	   local n=v.b:rows()
	   if r<=n then v=v.b else v,r=v.m,r-n end
  	   return v:item(r,c)
	 end,
}

function viewops:concat(v)
  local r=CustomView(concatenating)
  r.b,r.m=commoncols(self,v)
  return r
end

hasher={ff=tonumber('ffffffff',16)}

hasher['nil'] = function (v) return 0 end

function hasher.number(v)
  return v
end

function hasher.string(v)
  local h,s,n=0,1,strlen(v)
  if n>1000 then s=bor(n/100,0) end
  for i=1,n,s do h=bxor(h+h,strbyte(v,i)) end
  return h
end

function hasher.table(v)
  assert(nil) --XXX let's not group or sort on subviews just yet
  assert(tag(v)==viewtag or tag(v)==custviewtag)
  local h,m=0,buildhashmap(v)
  for i=1,getn(m) do h=h+m[i] end
  return h
end

function buildhashmap(v,l)
  if not l then l=intrun(v:cols()) end
  local m={n=v:rows()}
  if m.n>0 then
    for i=1,m.n do m[i]=0 end
    for i=1,getn(l) do
      local p=l[i]
      local t=type(v[1][p])
      local f=hasher[t]
      assert(f,t)
      for j=1,v:rows() do m[j]=m[j]+f(v[j][p]) end
    end
    local g,c,x,y={},{},{},{}
    for i=m.n,1,-1 do -- reversed loop so chains are in original order
      --local h=band(m[i],hasher.ff)
      local h=m[i]
      --XXX need to chase and split chains further on hash collisions
      --    counts can probably also be determined then (i.e. drop "c")
      if g[h] then c[h]=(c[h] or 1)+1 end
      m[i],g[h]=g[h],i
    end
    for k,v in g do tinsert(x,v); tinsert(y,c[k] or 1) end
    return x,y,m
  end
end

function docombine(v)
  local b=v.b
  if v.b~=v.c then b=b:concat(v.c) end
  local g,c,z=buildhashmap(b)
  v.m[2]=v.m[1](v,g,c,z)
  return v.m[2]
end

combining={
  cols = function (v) return v.b:cols() end,
  syms = function (v) return v.b:syms() end,
  rows = function (v) return getn(v.m[2] or docombine(v)) end,
  item = function (v,r,c)
  	   local t=v.m[2] or docombine(v)
  	   local x=t[r]
	   if x<0 then v,r=v.b,-x else v,r=v.c,x end
	   return v:item(r,c)
	 end,
}

function concatfix(x,n)
  if x<=n then return -x else return x-n end
end

function viewops:union(v)
  local r=CustomView(combining)
  local f=function (v,g,c,m)
	    local o,n={},v.b:rows()
	    for i=1,getn(g) do tinsert(o,concatfix(g[i],n)) end
	    return o
  	  end
  r.b,r.c=commoncols(self,v)
  r.m={f}
  return r
end

function viewops:except(v)
  local r=CustomView(combining)
  local f=function (v,g,c,m)
	    local o,n={},v.b:rows()
	    for i=1,getn(g) do
	      if g[i]<=n then
		local h=g[i]
	        repeat h=m[h] until not h or h>n
		if not h or h<=n then tinsert(o,concatfix(g[i],n)) end
	      end
	    end
	    return o
  	  end
  r.b,r.c=commoncols(self,v)
  r.m={f}
  return r
end

function viewops:intersect(v)
  local r=CustomView(combining)
  local f=function (v,g,c,m)
	    local o,n={},v.b:rows()
	    for i=1,getn(g) do
	      if g[i]<=n then
		local h=g[i]
	        repeat h=m[h] until not h or h>n
		if h and h>n then tinsert(o,concatfix(g[i],n)) end
	      end
	    end
	    return o
  	  end
  r.b,r.c=commoncols(self,v)
  r.m={f}
  return r
end

function viewops:distinct()
  local r=CustomView(combining)
  local f=function (v,g,c,m)
	    local o={}
	    for i=1,getn(g) do tinsert(o,g[i]) end
	    return o
  	  end
  r.b,r.c,r.m=self,self,{f}
  return r
end

grouping={
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
	     return v.b:item(bor((r-1)/n,0)+1,c) -- tricky floor()
	   end
	   return v.m:item(imod((r-1),n)+1,c-k)
	 end,
}

function viewops:group(f,l)
  local r=CustomView(grouping)
  assert(nil) --XXX not finished
  r.b,r.m=self,v
  return r
end

if not omitGroupDump then
  dofile('util.lua')
  serves:dump('serves')
  likes:dump('likes')
  serves:project{1}:distinct():dump('distinct')
  likes:concat(serves):dump('concat')
  likes:union(frequents):dump('union')
  likes:except(frequents):dump('except')
  likes:intersect(frequents):dump('intersect')
  serves:project{'beer'}:distinct():dump('s-beer')
  likes:project{'beer'}:distinct():dump('l-beer')
end
