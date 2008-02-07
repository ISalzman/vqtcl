#!/usr/bin/env lux

-- Bind SQL select's, modeled after MkSQL by Gordon McMillan
-- 22/02/2001 jcw@equi4.com

-- globals used in this code for now are:

omitSqlPrepareDemo=1
dofile('qprepare.lua')

qbind={}

function qbind.lit(x)
  return function (v,r) return %x end
end

function qbind.col(l,c)
  if l==0 then
    return function (v,r) return v[r][%c] end
  else
    return function (v,r)
      local x=v[r]
      for i=1,%l do x=rawget(x,'v'):prow() end
      return x[%c]
    end
  end
end

function qbind.AND(...)
  return function (v,r)
    local d=%arg
    for i=1,getn(d) do if not d[i](v,r) then return end end
    return 1
  end
end

function qbind.OR(...)
  return function (v,r)
    local d=%arg
    for i=1,getn(d) do if d[i](v,r) then return 1 end end
  end
end

function qbind.NOT(a)
  return function (v,r) return not %a(v,r) end
end

function qbind.exists(x)
  return  function (v,r) return %x(v,r):rows()>0 end
end

-- Shorthand, this is a function returning a function returning a function!
function binexpr(f)
  return function (a,b)
    local f=%f
    return function (v,r) return %f(%a(v,r),%b(v,r)) end
  end
end

qbind['+'] = binexpr(function (a,b) return a+b end)
qbind['-'] = binexpr(function (a,b) return a-b end)
qbind['*'] = binexpr(function (a,b) return a*b end)
qbind['/'] = binexpr(function (a,b) return a/b end)

qbind['='] = binexpr(function (a,b) return a==b end)
qbind['<'] = binexpr(function (a,b) return a<b end)
qbind['>'] = binexpr(function (a,b) return a>b end)

qbind['<>'] = binexpr(function (a,b) return a~=b end)
qbind['<='] = binexpr(function (a,b) return a<=b end)
qbind['>='] = binexpr(function (a,b) return a>=b end)

function qbind.eval(...)
  return function (v,r)
    local x
    for i=1,getn(%arg) do x=%arg[i](v,r) end
    return x
  end
end

function setcontext(f,v,r)
  local w=f(v,r)
  w:setprow(v[r])
  return w
end

function qbind.filter(d,c)
  return function (v,r) return setcontext(%d,v,r):filter(%c) end
end

function qbind.explode(a,b)
  return function (v,r) return %a(v,r):explode(%b(v,r)) end
end

function qbind.compute(d,...)
  return function (v,r) return setcontext(%d,v,r):compute(%arg) end
end

function qbind.union(a,b)
  return function (v,r) return %a(v,r):union(%b(v,r)) end
end

function qbind.except(a,b)
  return function (v,r) return %a(v,r):except(%b(v,r)) end
end

function qbind.intersect(a,b)
  return function (v,r) return %a(v,r):intersect(%b(v,r)) end
end

function qbind.group(d,p,...)
  return function (v,r)
    local w=View {%p}
    --? w:setprow(v[r])
    w:append(%d(v,r))
    return w
  end
end

function qbind.sum(c,e)
  return function (v,r)
    local w,s=%c(v,r),0
    --? w:setprow(v[r])
    for i=1,w:rows() do s=s+%e(w,i) end
    return s
  end
end

function qbind.avg(c,e)
  return function (v,r)
    local w,s=%c(v,r),0
    --? w:setprow(v[r])
    local n=w:rows()
    for i=1,n do s=s+%e(w,i) end
    return s/n
  end
end

function qbind.countrows(c)
  return function (v,r) return %c(v,r):rows() end
end

function bindsql(p)
  if p then
    local x=p[1]
    local q={}
    for i=2,getn(p) do
      local y=p[i]
      if type(y)=='table' then y=bindsql(y) end
      tinsert(q,y)
    end
    local f=qbind[x]
    assert(f,x)
    return call(f,q)
  end
end

----------------------------------------------------------------------------- 

dofile('util.lua')

local db=root[1]
local fmt=getschema(db)
--pretty('fmt',fmt)

n=0

if n then
  if n>0 then
    sql=examples[n]
    print(sql)
    r,v,ast=sqlprepare(sql,fmt)
    pretty('v',v)
    print()
    lispy('',r)
    print()
    f=bindsql(r)
    e=f(root,1)
    e:dump()
  else
    for i=1,getn(examples) do
      print('\nexample',i)
      print(examples[i])
      r,v,ast=sqlprepare(examples[i],fmt)
      --lispy('',r)
      --print()
      f=bindsql(r)
      e=f(root,1)
      e:dump()
    end
  end
else
  sql=[[
    SELECT perweek*2 twice, 2*twice quad FROM frequents
     WHERE twice=4 OR perweek*6=2*3
  ]]
  sql=[[
    SELECT 123 AS a, SUM(a*quantity) AS total, AVG(quantity),
           COUNT(*) AS tcount, total/tcount
     FROM serves
  ]]
  print(sql)
  r,v,ast=sqlprepare(sql,fmt)
  treedump(ast)
  print()
  pretty('v',v)
  print()
  lispy('',r)
  print()
  f=bindsql(r)
  e=f(root,1)
  e:dump()
end
