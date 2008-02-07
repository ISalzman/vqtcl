#!/usr/bin/env lux

-- Prepare SQL select's, modeled after MkSQL by Gordon McMillan
-- 22/02/2001 jcw@equi4.com

-- globals used in this code for now are: level, qmap

omitSqlResolveDemo=1
dofile('qresolve.lua')

--TODO get rid of the fixup stuff, see the 2 todo's below
qfixup={}

--TODO this should be moved to qresolve - no processing on nodes here!
function qfixup.col(c,d)
  local q,a=qmap,c.t..'.'..c.c
  repeat
    if q[a] then return {'col',level-q[a][2],q[a][1]} end
    q=q[0]
  until not q
  assert(q,a)
end

--TODO get rid of the d arg, which is only being used here
-- probably means that a bit more info needs to be stored in select nodes
-- once d arg is gone, this can be merged with qtoplevel.select
function qfixup.select(x,d)
  d[me][0]=d
  return qtoplevel.select(x,d[me])
end

function buildexpr(x,d)
  if type(x)~='table' then return x end
  local r={x.A}
  -- upper/lowercase messiness (for exists, not for the top level)
  if r[1]=='not' then r[1]='NOT' end
  for i=1,getn(x) do tinsert(r,buildexpr(x[i],d)) end
  if qfixup[x.A] then r=qfixup[x.A](x,d) end
  r.n=nil
  return r
end

--TODO try to get rid of qtoplevel, make then qbind functions instead
qtoplevel={}

function qtoplevel.select(x,d)
  local keepme=me
  me=x.c
  local m={[0]=qmap}
  qmap=m
  level=(level or 0)+1
  local r,o={'explode'},0
  for j=1,getn(x.f) do
    local k,l,e=x.f[j],level,d
    while not e[k] do e,l=e[0],l-1 end
    --XXX this seems like duplication of qfixup.col logic...
    for i=1,getn(e) do
      if e[i]==k then tinsert(r,{'col',level-l,i}) end
    end
    for i=1,getn(e[k]) do
      o=o+1
      m[k..'.'..e[k][i]]={o,level}
    end
  end
  assert(getn(r)>1)
  while getn(r)>3 do
    r[2]={'explode',r[2],r[3]}
    tremove(r,3)
  end
  if getn(r)==2 then r=r[2] end
  r.n=nil
  if x.g then
    r={'group',r}
    local g=gsub(x.c,'Q','G')
    tinsert(r,g)
    --XXX XXX XXX o=o+1
    o=1
    m[x.c..'.'..g]={o,level}
  end
  r={'compute',r}
  if x.s then
    for i=1,getn(x.s) do
      if x.s[i].A~='*' then
	o=o+1
	local y,n=x.s[i],'A~'..o
	if y.A=='alias' then
	  y,n=y[1],y.t
	end
	  tinsert(r,n)
	tinsert(r,buildexpr(y,d))
	m[x.c..'.'..n]={o,level}
      end
    end
  end
  if getn(r)==2 then r=r[2] end
  r.n=nil
  if x.w then r={'filter',r,buildexpr(x.w,d)} end
  level=level-1
  qmap=m[0]
  me=keepme
  return r
end

function setoperation(x,d)
  local r={x.A}
  for i=1,getn(x) do
    local q=qtoplevel[x[i].A]
    assert(q,x[i].A)
    tinsert(r,q(x[i],d))
  end
  r.n=nil
  return r
end

qtoplevel.union=setoperation
qtoplevel.except=setoperation
qtoplevel.intersect=setoperation

function sqlprepare(sqlin,dbd)
  local v,ast=sqlresolve(sqlin,dbd)
  local r={'eval'}
  for i=1,getn(v) do
    local q=v[i]
    assert(qtoplevel[q.A],q.A)
    tinsert(r,qtoplevel[q.A](q,dbd))
  end
  if getn(r)==2 then r=r[2] end
  r.n=nil
  return r,v,ast
end

----------------------------------------------------------------------------- 

if not omitSqlPrepareDemo then
  dofile('util.lua')

  local db=root[1]
  local fmt=getschema(db)
  --pretty('fmt',fmt)

  for i=1,getn(examples) do
    print('\nexample',i)
    print(examples[i])
    r,v,ast=sqlprepare(examples[i],fmt)
    lispy('',r)
  end
end
