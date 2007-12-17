T = lvq
V = 1.6.0
CONFIG = ./config

include $(CONFIG)

B = ../base
DEPS = src/$T.c src/vq_conf.h $B/vlerq.c $B/vlerq.h $B/v_defs.h $B/vload.c \
	$B/vmutable.c $B/vbuffer.c $B/vsave.c $B/vranges.c $B/vreader.c \
	$B/vopdef.c $B/vload.h $B/vmutable.h $B/vopdef.h $B/vranges.h \
	$B/vsave.h $B/vreader.h $B/vbuffer.h $B/vdispatch.c $B/vdispatch.h
	
OBJS = src/$T.o
LUAS = src/$T.lua

lib: src/$(LIBNAME)

src/$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) $(LIB_OPTION) -o src/$(LIBNAME) $(OBJS)

src/$T.o: $(DEPS) Makefile config

$B/vdispatch.c: ../etc/mkdispatch.tcl
	cd ../etc && make dispatch

install: src/$(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/$T
	cp src/$(LIBNAME) $(LUA_LIBDIR)/$T/core.so
	mkdir -p $(LUA_DIR)
	cp $(LUAS) $(LUA_DIR)

clean:
	rm -f src/$(LIBNAME) $(OBJS)
