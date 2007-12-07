T = lvq
V = 1.6.0
CONFIG = ./config

include $(CONFIG)

C = ../core
SRCS = src/$T.c $C/vlerq.c $C/vlerq.h $C/def.h $C/opdef.c $C/reader.c \
	$C/load.c $C/buffer.c $C/save.c $C/ranges.c $C/mutable.c
OBJS = src/$T.o
LUAS = src/$T.lua

lib: src/$(LIBNAME)

src/$(LIBNAME): $(OBJS)
	export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) $(LIB_OPTION) -o src/$(LIBNAME) $(OBJS)

src/$T.o: $(SRCS) Makefile config

install: src/$(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/$T
	cp src/$(LIBNAME) $(LUA_LIBDIR)/$T/core.so
	mkdir -p $(LUA_DIR)
	cp $(LUAS) $(LUA_DIR)

clean:
	rm -f src/$(LIBNAME) $(OBJS)
