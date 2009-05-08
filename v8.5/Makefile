CPATH = \
  src/?.so;./?.so;/usr/local/lib/lua/5.1/?.so;/usr/local/lib/lua/5.1/loadall.so

all: testall

# run both the quick and the regular tests
test: build
	tests/run tests/*_[qt].lua

# all tests, including more extensive/slower ones
testall: build
	tests/run tests/*_[qtx].lua

# use LuaJIT to run all tests
testjit: build
	luajit tests/run tests/*.lua

# called on F6 by TextMate for MacOSX, only runs quick tests, no src build
testmate:
	@tests/run tests/*_q.lua | tail -1

build clean:
	cd src && make $@
