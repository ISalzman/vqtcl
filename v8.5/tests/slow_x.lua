require 'lunit'

----------------------------------------------------- slow tests (not used yet)

module('slow', package.seeall, lunit.testcase)

function setup()
	a = 1
	s = 'hop' 
end
function teardown()
	-- some tearDown() code if necessary
end

function test1()
  assert_equal( 1, a )
end
function test2()
  assert_equal( 1, a )
  assert_equal( 'hop', s )
  assert( s ~= 'bof' )
end
function xtest3() --XXX a
  assert_equal( 1, a )
  assert_equal( 'hop', s )
end
