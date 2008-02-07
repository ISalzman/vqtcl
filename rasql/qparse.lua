#!/usr/bin/env lux

-- Parse SQL select's, modeled after MkSQL by Gordon McMillan
-- 20/02/2001 jcw@equi4.com

-- globals used in this code for now are: input, token, and pos

-- grab the next token off the input string
function tokenize()
  local b,e,t=strfind(input,'^%s*(%w+)',pos) -- numbers and identifiers
  if not b then
    b,e,t=strfind(input,"^%s*(%d+%.%d*)",pos) -- floating point numbers
    if not b then
      b,e,t=strfind(input,"^%s*('[^'\n']*')",pos) -- quoted literals
      if not b then
	b,e,t=strfind(input,"^%s*([<>=][=>]?)",pos) -- comparisons
	if not b then
	  b,e,t=strfind(input,"^%s*(.?)",pos) -- other operators
	end
      end
    end
  end
  pos=e+1
  --print('GOT',t)
  return t
end
-- only grab a token if there is no pending one
function peek()
  if not token then token=tokenize() end
  if token~='' then return token end
end
-- grab and consume the next token if it matches
function match(s)
  if peek()==s then token=nil; return s end
end
-- make sure the next token matches and consume it
function require(s)
  assert(match(s),'require '..s..', got '..(token or '<eof>'))
end
-- utility for parsing binary operators
function binary(fun,a,b)
  local r=fun()
  while peek()==a or token==b do
    r={op='BINOP',x=match(token);r,fun()}
  end
  return r
end

function p_stmtlist()
  local r={op='STMTLIST'}
  repeat tinsert(r,p_stmt()) until not match(';')
  assert(not peek(),token)
  return r
end

function p_stmt()
  if match('SELECT') then return p_query() end
  --if match('UPDATE') then return p_update() end
  --if match('DELETE') then return p_delete() end
  --if match('INSERT') then return p_insert() end
  --if match('CREATE') then return p_create() end
  --if match('COMMIT') then return p_commit() end
  assert(nil)
end

function p_query()
  local r=p_subquery()
  while 1 do
    local t=match('EXCEPT') or match('INTERSECT') or match('UNION')
    if not t then break end
    require('SELECT')
    r={op=t;r,p_subquery()}
  end
  if match('ORDER') then require('BY'); r.o=p_orderbyspec() end
  return r
end

function p_subquery()
  local r={op='SELECT',f=p_select_list()}
  require('FROM')
  r.t=p_table_list()
  if match('WHERE') then r.w=p_condition() end
  if match('GROUP') then require('BY'); r.g=p_groupspec() end
  if match('HAVING') then r.h=p_condition() end
  return r
end

function p_select_list()
  local r={op='SELECTLIST'}
  repeat
    local r2=match('*')
    if not r2 then
      r2=p_expr()
      if match('.') then
        require('*')
	r2={op='ALLCOLSOF',t=r2}
      else
	match('AS')
	if peek() and strfind(token,'[a-z]') then --XXX names lowercase
	  r2={op='ALIAS',a=match(token);r2}
	end
      end
    end
    tinsert(r,r2)
  until not match(',')
  return r
end

function p_table_list()
  local r={op='TABLELIST'}
  repeat
    local r2={op='TABLE',t=match(peek())}
    match('AS')
    if peek() and strfind(token,'[a-z]') then --XXX names lowercase
      r2.a=match(token)
    end
    tinsert(r,r2)
  until not match(',')
  return r
end

function p_groupspec()
  local r={op='GROUPSPEC'}
  repeat tinsert(r,p_col_ref()) until not match(',')
  return r
end

function p_col_ref()
  local r=match(peek())
  if match('.') then r={op='COLREF',t=r,c=match(peek())} end
  return r
end

function p_condition()
  return binary(p_condition_term,'OR')
end

function p_condition_term()
  return binary(p_condition_factor,'AND')
end

function p_condition_factor()
  if match('NOT') then return {op='NOT';p_condition_primary()} end
  return p_condition_primary()
end

function p_condition_primary()
  if match('(') then
    local r=p_condition()
    require(')')
    return r
  end
  if match('EXISTS') then
    require('(')
    require('SELECT')
    local q=p_subquery()
    require(')')
    return {op='EXISTS';q}
  end
  local r=p_expr()
  local t=peek()
  if t=='<' or t=='<=' or t=='>' or t=='>=' or t=='=' or t=='<>' then
    return {op='CMP',r=match(token);r,p_expr()}
  end
  local f=match('NOT')
  if match('BETWEEN') then
    local e=p_expr()
    require('AND')
    return {op='BETWEEN',f=f;r,e,p_expr()}
  end
  if match('IN') then
    require('(')
    local l=p_literal_list()
    require(')')
    return {op='IN',f=f;r,l}
  end
  assert(not f)
  return r
end

function p_expr()
  return binary(p_expr_term,'+','-')
end

function p_expr_term()
  return binary(p_expr_factor,'*','/')
end

function p_expr_factor()
  if match('COUNT') then
    require('(')
    local r=match('*') or p_expr()
    require(')')
    if r=='*' then return {op='COUNTROWS'} end
    return {op='COUNT';r}
  end
  local t=match('AVG') or match('SUM') or match('TOTAL')
  if t then require('(')
    local r=p_expr()
    require(')')
    return {op=t;r}
  end
  if match('(') then
    local r=p_expr()
    require(')')
    return r
  end
  return p_col_ref()
end

function p_literal_list()
  local r={op='LITERALLIST'}
  repeat tinsert(r,match(peek())) until not match(',')
  return r
end

function p_orderbyspec()
  local r={op='ORDERBYSPEC'}
  repeat
    local r2=p_col_ref()
    local t=match('ASC') or match('DESC')
    if t then r2={op='ORDEREL',d=t;r2} end
    tinsert(r,r2)
  until not match(',')
  return r
end

function sqlparse(s)
  input,pos,token=s,1,nil
  return p_stmtlist()
end
 
----------------------------------------------------------------------------- 

if not omitSqlParseUtils then
  function treedump(t,s)
    local s=s or ' | '
    if type(t)=='table' then
      print(s..t.op)
      s=' |'..strrep(' ',strlen(s)-2)
      for i=1,getn(t) do
	treedump(t[i],s..i..'. ')
      end
      for i,v in t do
	if type(i)~='number' and i~='n' and i~='op' then
	  treedump(v,s..i..'  ')
	end
      end
    else
      print(s..t)
    end
  end
end

if not omitSqlParseExamples then
  examples={} -- examples from Gadfly by Aaron Watters

  examples[1]=[[
    SELECT * FROM frequents
     WHERE drinker = 'norm'
  ]]
  examples[2]=[[
    SELECT drinker FROM likes
     UNION
     SELECT drinker FROM frequents
  ]]
  examples[3]=[[
    SELECT drinker FROM likes
     EXCEPT
     SELECT drinker FROM frequents
  ]]
  examples[4]=[[
    SELECT * FROM frequents
     WHERE drinker>'norm' OR drinker<'b'
  ]]
  examples[5]=[[
    SELECT *
     FROM frequents AS f, serves AS s
     WHERE f.bar = s.bar
  ]]
  examples[6]=[[
    SELECT *
     FROM frequents AS f, serves AS s
     WHERE f.bar = s.bar AND
       NOT EXISTS(
	 SELECT l.drinker, l.beer
	 FROM likes l
	 WHERE l.drinker=f.drinker AND s.beer=l.beer)
  ]]
  examples[7]=[[
    SELECT SUM(quantity) AS total, AVG(quantity),
	   COUNT(*) AS tcount, total/tcount
     FROM serves
  ]]
  examples[8]=[[
    SELECT bar, SUM(quantity) AS total, AVG(quantity),
	   COUNT(*) AS beers, total/beers
     FROM serves
     WHERE beer <> 'bud'
     GROUP BY bar
     HAVING total>500 OR beers>3
     ORDER BY 2, bar
  ]]
  examples[9]=[[
    SELECT l.drinker, l.beer, COUNT(*), SUM(l.perday*f.perweek)
     FROM likes l, frequents f
     WHERE l.drinker=f.drinker
     GROUP BY l.drinker, l.beer
     ORDER BY 4 DESC, l.drinker, l.beer
  ]]
end

if not omitSqlParseDemo then
  for i=1,getn(examples) do
    local e=examples[i]
    print(e)
    treedump(sqlparse(e))
  end
end
