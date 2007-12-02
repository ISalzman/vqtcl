T = lvq
V = 1.6.0
CONFIG = ./config

include $(CONFIG)

SRCS = src/$T.c src/$T.h src/vcore.c src/vcore.h src/vdefs.h src/vopdef.c
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
