require 'lunit'
require 'vq'

----------------------------------------------------------------- minimal views

module('more.minimal', package.seeall, lunit.testcase)

function setup()
  v = vq.newview(nil, 2)
end

function testEmpty()
  assert( is_userdata(v) )
end
function testNoCols()
  assert_equal( 2, #v )
end
function testMeta()
  assert( is_userdata(v:meta()) )
  assert_equal( 3, #v:meta() )
end
function testMetaMeta()
  assert( is_userdata(v:meta():meta()) )
  assert_equal( 3, #v:meta():meta() )
end
function testMetaContents()
  local m = v:meta()
  assert_equal( 'name', m[1][1] )
  assert_equal( vq.S, m[1][2] )
  assert_equal( 0, #m[1][3] )
  assert_equal( 'type', m[2][1] )
  assert_equal( vq.I, m[2][2] )
  assert_equal( 0, #m[2][3] )
  assert_equal( 'subv', m[3][1] )
  assert_equal( vq.V, m[3][2] )
  assert_equal( 3, #m[3][3] )
end

-------------------------------------------------------------- custom meta-view

module('more.custom', package.seeall, lunit.testcase)

function setup()
  v = vq.newview(nil, 2)
  v[1][1] = 'name'
  v[1][2] = vq.S
  v[2][1] = 'age'
  v[2][2] = vq.I
end

function testMetaView()
  assert_equal( 'name', v[1][1] )
  assert_equal( vq.S, v[1][2] )
  assert_equal( 'age', v[2][1] )
  assert_equal( vq.I, v[2][2] )
end
function testCustomView()
  local w = vq.newview(v, 2)
  assert_equal( 2, #w )
  assert_equal( 2, #w:meta() )
  w[1][1] = 'Joe'
  w[1][2] = 66
  w[2][1] = 'Sarah'
  w[2][2] = 44
  assert_equal( 'Joe', w[1][1] )
  assert_equal( 66, w[1][2] )
  assert_equal( 'Sarah', w[2][1] )
  assert_equal( 44, w[2][2] )
end
