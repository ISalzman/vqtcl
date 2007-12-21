T = lvq
V = 1.6.0
CONFIG = ./config

include $(CONFIG)

B = ../base
DEPS = $T.c vq_conf.h $B/vlerq.c $B/vlerq.h $B/v_defs.h $B/vload.c \
	$B/vmutable.c $B/vbuffer.c $B/vsave.c $B/vranges.c $B/vreader.c \
	$B/vopdef.c $B/vload.h $B/vmutable.h $B/vopdef.h $B/vranges.h \
	$B/vsave.h $B/vreader.h $B/vbuffer.h $B/vdispatch.c $B/vdispatch.h \
	$B/vutil.h $B/vutil.c
	
OBJS = $T.o
LUAS = $T.lua

lib: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) $(LIB_OPTION) -o $(LIBNAME) $(OBJS)

$T.o: $(DEPS) Makefile $(CONFIG)

$B/vdispatch.c: ../etc/mkdispatch.tcl
	cd ../etc && make dispatch

install: $(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/$T
	cp $(LIBNAME) $(LUA_LIBDIR)/$T/core.so
	mkdir -p $(LUA_DIR)
	cp $(LUAS) $(LUA_DIR)

clean:
	rm -f $(LIBNAME) $(OBJS)
