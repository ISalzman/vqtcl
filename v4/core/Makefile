# Makefile - Build the Vlerq Tcl extension on Mac OS X and some other platforms
#
# NOTE: when this file or an include dependency changes, do "make clean"

GEN_SRCS = bits.c buffer.c cmds.c column.c emit.c file.c getters.c hash.c \
 	   indirect.c loop.c mutable.c sorted.c view.c wrap_gen.c
TCL_SRCS = colobj.c ext_tcl.c viewobj.c

# support compilation on a few platforms, the default is for Linux
#   on Mac OS X: include "ARCHSUFFIX=.macosx" in the environment
#   for Windows: include "ARCHSUFFIX=.win32" in the environment

ifeq ($(ARCHSUFFIX),)
# find Tcl headers on Ubuntu Linux:
	CFLAGS += -I/usr/include/tcl8.4 -fPIC -fomit-frame-pointer -Wno-long-long
	dll = .so
	dyn = -shared
endif
ifeq ($(ARCHSUFFIX),.macosx)
	dll = .dylib
	dyn = -dynamiclib -fno-common -exported_symbols_list make.syms
endif
ifeq ($(ARCHSUFFIX),.win32)
	CC = gcc
# find Tcl headers when cross-compiling via xmingw32.sh:
	CFLAGS += -I/System/Library/Frameworks/Tcl.framework/Headers
	CFLAGS += -fomit-frame-pointer -Wno-long-long
	dll = .dll
	dyn = -shared
endif

CFLAGS += -DDEBUG -Isrc_tcl -g -Wall -pedantic -Wstrict-prototypes -O0
TARGET = libvlerq4$(dll)
VPATH = src_tcl:src
RATDIR = ../ratcl/lib/vlerq4
RBYDIR = ../ruby
TEADIR = ../tcl
OBCDIR = ../objc
LUADIR = ../lua
PYTDIR = ../python
K64DIR = ../kitgen/8.4/kit-b64

SRCS = $(GEN_SRCS:%=src/%) $(TCL_SRCS:%=src_tcl/%)
OBJS = $(GEN_SRCS:.c=.o) $(TCL_SRCS:.c=.o)

.PHONY: all test smoke crosstest rat clean docs macosx macppc mac_64 win32 \
	release linux puppie-fixclock puppie teevie drakie bunnie gennie \
	tea ruby python objc lua bindings debbie-fixclock debbie etchie

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo Linking $(TARGET)
	@$(CC) $(dyn) -o $@ $(OBJS) $(LDFLAGS)
	@echo OK

test: $(TARGET)
	tests/all.tcl

smoke: $(TARGET)
	tests/basic.test

release: macppc linux win32 test
	ls -l libvlerq4.*.gz

# macppc uses a universal build and a ppc binary to test in a big-endian context
 macppc: macosx
	cp build/Release/libvlerq4.dylib .
	tclkit84ppc tests/all.tcl
	rm libvlerq4.dylib
	
# mac_64 uses a 64-bit intel binary built by kitgen to test in a 64-bit context
# must update kitgen and use tests from tea dir to get the version numbers right
 mac_64: tea
	cd $(K64DIR) && rm -rf build/vlerq && make
	$(K64DIR)/tclkit-cli $(TEADIR)/tests/all.tcl

# macosx builds an optimized ppc/ppc64/i386/x86_64 universal library using xcode
 macosx:
	xcodebuild -project vlerq.xcodeproj -configuration Release && \
	  cp build/Release/libvlerq4.dylib . && gzip -f libvlerq4.dylib

# win32 builds a dll using a mingw cross-compiler from macosx
 win32: clean
	( . xmingw32.sh; \
	  make && strip libvlerq4.dll && gzip -f libvlerq4.dll; \
	  make clean )

# linux builds use remote machines, though puppie & debbie run under a vm
 linux: debbie-fixclock debbie
	mv libvlerq4.so-debbie.gz libvlerq4.so.gz
 debbie-fixclock:
	setdate-ssh debbie
 puppie-fixclock:
	setdate-ssh puppie
# bunnie: Ubuntu, drakie: Mandrake 7.2, gennie: Gentoo, puppie: Puppy Linux,
# teevie: Ubuntu 64b, debbie: Debian 3.1, etchie: Debian 4.0
# these targets expect to find a tclkit in the path to run the tests
 bunnie debbie drakie etchie gennie puppie teevie: clean
	rsync -a --exclude=libvlerq4.* --exclude=testfiles.tmp . $@:/tmp/vlerq
	sleep 2 ;# avoid clock skew problems
	ssh $@ 'cd /tmp/vlerq && make clean && PATH=~/bin:$$PATH make test && \
		strip libvlerq4.so && gzip -f libvlerq4.so'
	rsync -a $@:/tmp/vlerq/libvlerq4.so.gz libvlerq4.so-$@.gz

# build binaries for multiple platforms for the ratcl.kit package
rat:
	gzip -d <libvlerq4.dll.gz >$(RATDIR)/libvlerq4.dll
	gzip -d <libvlerq4.dylib.gz >$(RATDIR)/libvlerq4.dylib
	gzip -d <libvlerq4.so.gz >$(RATDIR)/libvlerq4.so
	rsync -a src_tcl/{m2mvfs,mkclvfs,mklite,ratcl}.tcl $(RATDIR)/
	
# generate a merged source file for the TEA3 Tcl extension
tea:
	etc/includer.tcl etc/tcl.c >$(TEADIR)/generic/vlerq.c
	rsync -a --exclude=.svn data tests $(TEADIR)
	rsync -a src_tcl/m2mvfs.tcl src_tcl/mkclvfs.tcl \
		  src_tcl/mklite.tcl src_tcl/ratcl.tcl $(TEADIR)/library/

# generate a merged source file for the Ruby extension
ruby:
	etc/includer.tcl etc/ruby.h >$(RBYDIR)/ext/vlerq.h
	etc/includer.tcl etc/ruby.c >$(RBYDIR)/ext/vlerq.c
	rsync -a src_ruby/vlerq_ext.h $(RBYDIR)/ext/vlerq_ext.h

# generate a merged source file for the Python extension
python:
	etc/includer.tcl etc/python.h >$(PYTDIR)/vlerq.h
	etc/includer.tcl etc/python.c >$(PYTDIR)/vlerq.c

# generate a merged source file for the Objective-C extension
objc:
	etc/includer.tcl etc/objc.h >$(OBCDIR)/vlerq.h
	etc/includer.tcl etc/objc.c >$(OBCDIR)/vlerq.c

# generate a merged source file for the Lua extension
lua:
	etc/includer.tcl etc/lua.h >$(LUADIR)/vlerq.h
	etc/includer.tcl etc/lua.c >$(LUADIR)/vlerq.c

# generate all bindings and run some quick tests
bindings: tea ruby python objc lua
	cd $(TEADIR) && sh configure && make test
	cd $(RBYDIR) && rake test
	cd $(PYTDIR) && python setup.py build && try.py
	cd ../js/mywkpi && xcodebuild -configuration Release
	
# grind does a valgrind run on puppie, with startup messages suppressed
grind: puppie
	ssh puppie 'cd /tmp/vlerq && make && \
	  valgrind -q --suppressions=/home/jcw/vg.supp --leak-check=full \
	           ~/kitgen/8.4/base-std/build/bin/tclsh8.4 tests/all.tcl'

# this is not for general use, due to the custom script and hard-wired paths
docs:
	markdown-tm 'Vlerq tech notes' \
	  <README >~/Sites/www.equi4.com/vlerqnotes.html
	
clean:
	rm -f $(OBJS) $(TARGET)
ifeq ($(ARCHSUFFIX),.macosx)
	makedepend -f- -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null | \
		sed 's|^src[_a-z]*/||' >make.deps
endif

-include make.deps
