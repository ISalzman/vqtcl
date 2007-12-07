T = lvq
V = 1.6.0
CONFIG = ./config

include $(CONFIG)

C = ../core
DEPS = src/$T.c src/vq_conf.h $C/vlerq.c $C/vlerq.h $C/v_defs.h $C/vmutable.c \
	$C/vload.c $C/vbuffer.c $C/vsave.c $C/vranges.c $C/vreader.c \
	$C/vopdef.c $C/vload.h $C/vmutable.h $C/vopdef.h $C/vranges.h \
	$C/vsave.h $C/vreader.h $C/vbuffer.h
	
OBJS = src/$T.o
LUAS = src/$T.lua

lib: src/$(LIBNAME)

src/$(LIBNAME): $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/$(LIBNAME) $(OBJS)

src/$T.o: $(DEPS) Makefile config

install: src/$(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/$T
	cp src/$(LIBNAME) $(LUA_LIBDIR)/$T/core.so
	mkdir -p $(LUA_DIR)
	cp $(LUAS) $(LUA_DIR)

clean:
	rm -f src/$(LIBNAME) $(OBJS)
