#!/usr/bin/env lux

-- Resolve SQL select's, modeled after MkSQL by Gordon McMillan
-- 22/02/2001 jcw@equi4.com

-- globals used in this code for now are: seqno, schema, and context

omitGroupDump=1
dofile('qgroup.lua')
omitSqlParseDemo=1
dofile('qparse.lua')

prep={}

function prep.ALIAS(x)
  local a=resolve(x[1])
  return {A='alias',t=x.a,n=1;a}
end

function prep.COLREF(x)
  local t=findtable(x.t,context.f)
  assert(t,x.t)
  return {A='col',t=t,c=x.c}
end

function mergenodes(x)
  local r={A=x.A}
  for i=1,getn(x) do
    if x[i].A==r.A then
      for j=1,getn(x[i]) do tinsert(r,x[i][j]) end
    else
      tinsert(r,x[i])
    end
  end
  return r
end

NaryMerge={['OR']=1,['AND']=1}

function prep.BINOP(x)
  local r={A=x.x,n=2;resolve(x[1]),resolve(x[2])}
  if NaryMerge[x.x] then r=mergenodes(r) end
  return r
end

function aggregate(x)
  context.g=context.g or {} -- force grouping
  local r=descend(x)
  local c=gsub(context.c,'Q','G')
  tinsert(r,1,{A='col',t=context.c,c=c})
  return r;
end

prep.AVG = aggregate
prep.SUM = aggregate
prep.TOTAL = aggregate
prep.COUNT = aggregate
prep.COUNTROWS = aggregate

function prep.CMP(x)
  return {A=x.r,n=2;resolve(x[1]),resolve(x[2])}
end

function prep.ORDEREL(x)
  return {A='orderel',d=x.d,n=1;resolve(x[1])}
end

-- list of names imported into this query, optionally aliased
function fromlist(tl)
  assert(tl.op=='TABLELIST')
  local t={}
  for i=1,getn(tl) do
    local x=tl[i]
    assert(x.op=='TABLE')
    t[i],t[x.a or x.t]=x.t,x.t
  end
  return t
end

function selectlist(sl,a)
  assert(sl.op=='SELECTLIST')
  local t={}
 if nil then --XXX disable two-pass for now, since it permits cycles
  -- first go through to make sure all alias names are known
  for i=1,getn(sl) do
    local x=sl[i]
    if type(x)=='table' and x.op=='ALIAS' then tinsert(a,x.a) end
  end
  -- then go through, resolving everything along the way
  for i=1,getn(sl) do
    local c=resolve(sl[i])
    tinsert(t,c)
  end
 else --XXX the single-pass version, this does not resolve forward aliases
  for i=1,getn(sl) do
    local c=resolve(sl[i])
    tinsert(t,c)
    if c.A=='alias' then tinsert(a,c.t) end
  end
 end --XXX
  return t
end

-- The fields in SELECT nodes are:
--  .A = 'SELECT'
--  .a = list of column aliases defined in .s (this is also in schema)
--  .c = unique name given to results of this query
--  .f = from list (can be used for lookup as well as int-indexed)
--  .g = groupby list
--  .h = having conditions
--  .o = orderby list
--  .s = selection list
--  .w = where conditions

function prep.SELECT(x)
  -- construct a new table name for query results
  seqno=seqno+1
  local q={A='select',c='Q~'..seqno,f=fromlist(x.t),a={}}
  -- fix things so that query results are almost like a real table
  -- the one difference is that they only contain aliased names
  q.f[getn(q.f)+1]=q.c
  q.f[q.c]=q.c
  -- push new context, i.e. chain of fromlists
  local k=context
  q.f[0],context=context.f,q
  -- adjust schema to include the results of this query
  schema[q.c]=q.a
  schema[getn(schema)+1]=q.c
  -- set the parent, but only while this function is running
  q.a[0]=schema
  schema=q.a
  local sq=seqno
  -- now start resolving things, first the selection list
  q.s=selectlist(x.f,q.a)
  if x.w then q.w=resolve(x.w) end
  if x.g then q.g=resolve(x.g); q.g.A=nil end
  if x.h then q.h=resolve(x.h) end
  if x.o then q.o=resolve(x.o); q.o.A=nil end
  -- restore previous state
  q.f[getn(q.f)]=nil
  q.f[q.c]=nil
  schema=schema[0]
  q.a[0]=nil
  context=k
  return q
end

function findtable(x,t)
  while t do
    if t[x] then return t[x] end
    t=t[0]
  end
end

function findcol(x)
  local t,s,r=context.f,schema,{}
  while t do
    s=s[0]
    for i=1,getn(t) do
      local v=t[i]
      for i=1,getn(s[v]) do
	if s[v][i]==x then tinsert(r,v) end
      end
    end
    assert(getn(r)<2,'column name ambiguous: '..x)
    if getn(r)>=1 then return r[1] end
    t=t[0]
  end
end

function descend(x)
  local t={A=strlower(x.op)}
  for i=1,getn(x) do tinsert(t,resolve(x[i])) end
  return t
end

function resolve(x)
  if type(x)=='string' then
    if strfind(x,'^[a-z]') then
      local c=findcol(x)
      assert(c,x)
      return {A='col',t=c,c=x}
    end
    if strfind(x,'^[0-9]') then return {A='lit',n=1;tonumber(x)} end
    if strsub(x,1,1)=="'" then return {A='lit',n=1;strsub(x,2,-2)} end
    if x=='*' then return {A=x} end
  end
  local f=prep[x.op] or descend
  return f(x)
end

function sqlresolve(sqlin,dbd)
  local ast=sqlparse(sqlin)
  schema=dbd
  seqno=0
  context={}
  local r=resolve(ast)
  r.A=nil
  return r,ast
end

function getschema(db)
  local s,t=rawget(db,'v'):syms(),{}
  for i=1,getn(s) do
    t[i]=s[i]
    t[s[i]]=db[i]:syms()
    t[s[i]][0]=t
  end
  return t
end

----------------------------------------------------------------------------- 

if not omitSqlResolveDemo then
  dofile('util.lua')

  local db=root[1]

  for i=1,getn(examples) do
  --for i=6,6 do
    local fmt=getschema(db)
    print('\n<<<example '..i..'>>>')
    print(examples[i])
    v,ast=sqlresolve(examples[i],fmt)
    --treedump(ast)
    --print()
    pretty('v',v)
    print()
    pretty('fmt',fmt)
  end
end
