require 'lunit'

--------------------------------------------- smoke test of the 'lunit' package

module('basic.smoke', package.seeall, lunit.testcase)

function setup()
  a = 1
  s = 'hop' 
end
function teardown()
	assert(s)
  assert(a)
end

function test1()
  assert_equal( 1, a )
end
function test2()
  assert_equal( 1, a )
  assert_equal( 'hop', s )
  assert( s ~= 'bof' )
end

-------------------------- verify loadability and basic use of the 'vq' package

module('basic.loaded', package.seeall, lunit.testcase)

function setup()
  require 'vq'
end

function testLoaded()
  assert( is_table(vq) )
end
function testConstants()
  assert_equal( 'LuaVlerq 1.9.0', vq._VERSION )
  assert_equal( 'Copyright (C) 1996-2008 Jean-Claude Wippler', vq._COPYRIGHT )
end

----------------------------------------------- simple int vector setup and use

module('basic.intvec', package.seeall, lunit.testcase)

function setup()
  require 'vq'
  v = vq.newcolumn(vq.I, 100)
end

function testNewvec()
  assert( v )
end
function testVecType()
  assert( is_userdata(v) )
end
function testVecLen()
  assert_equal( 100, #v )
end
function testGetSet()
  v[1] = 111
  assert_equal( 111, v[1] )
end
function testFiveSlots()
  for i = 1, 100 do
    v[i] = i * i
  end
  assert_equal( 1, v[1] )
  assert_equal( 25, v[5] )
  assert_equal( 100, v[10] )
  assert_equal( 2500, v[50] )
  assert_equal( 10000, v[100] )
end
function testInfo()
  local name, type, cpos = vq.info(v)
  assert_equal( "intvec", name )
  assert_equal( vq.I, type )
  assert_equal( 0, cpos )
end
function testRefs()
  assert_equal( 1, vq.refs(v) )
end

------------------------------------------------------ other basic vector types

module('basic.types', package.seeall, lunit.testcase)

function setup()
  require 'vq'
end

function testNils()
  v = vq.newcolumn(vq.N, 8)
  assert( v )
  assert_equal( 8, #v )
  v[1] = '?'
  assert( not v[1] )
end
function testInts()
  v = vq.newcolumn(vq.I, 9)
  assert( v )
  assert_equal( 9, #v )
  v[1] = -123
  assert_equal( -123, v[1] )
end
function testPositions()
  v = vq.newcolumn(vq.P, 10)
  assert( v )
  assert_equal( 10, #v )
  v[1] = 1234
  assert_equal( 1234, v[1] )
end
function testLongs()
  v = vq.newcolumn(vq.L, 11)
  assert( v )
  assert_equal( 11, #v )
  v[1] = 9876543210
  assert_equal( 9876543210, v[1] )
end
function testFloats()
  v = vq.newcolumn(vq.F, 12)
  assert( v )
  assert_equal( 12, #v )
  v[1] = 1234.5
  assert_equal( 1234.5, v[1] )
end
function testDoubles()
  v = vq.newcolumn(vq.D, 13)
  assert( v )
  assert_equal( 13, #v )
  v[1] = 1234.56789
  assert_equal( 1234.56789, v[1] )
end
function testStrings()
  v = vq.newcolumn(vq.S, 14)
  assert( v )
  assert_equal( 14, #v )
  v[1] = 'abc'
  assert_equal( 'abc', v[1] )
end
function testBytes()
  v = vq.newcolumn(vq.B, 15)
  assert( v )
  assert_equal( 15, #v )
  v[1] = 'xyz'
  v[2] = 'a\0b\0c'
  assert_equal( 'xyz', v[1] )
  assert_equal( 'a\0b\0c', v[2] )
end
function testAny()
  v = vq.newcolumn(vq.V, 16)
  assert( v )
  assert_equal( 16, #v )
  v[1] = vq.newview(nil, 123)
  local v1 = v[1]
  assert_equal( 123, #v1 )
  local name, type, cnum = vq.info(v1)
  assert_equal( 'nestedvec', name )
  assert_equal( vq.V, type )
  assert_equal( 0, cnum )
  -- reference counts won't be accurate until garbage has been collected
  assert( vq.refs(v1) >= 2 and vq.refs(v1) <= 3 )
  collectgarbage('collect')
  assert_equal( 2, vq.refs(v1) )
end
