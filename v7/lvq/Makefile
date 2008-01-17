T = lvq
V = 1.7.0
CONFIG = ./config

include $(CONFIG)

DEPS = $T.c vlerq.h defs.h vops.c ranges.c
OBJS = $T.o
LUAS = $T.lua

$(LIBNAME): $(OBJS)
	$(CC) $(LIB_OPTION) -o $(LIBNAME) $(OBJS)

$T.o: $(DEPS) Makefile $(CONFIG)

install: $(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/$T
	cp $(LIBNAME) $(LUA_LIBDIR)/$T/core.so
	mkdir -p $(LUA_DIR)
	cp $(LUAS) $(LUA_DIR)

test: $(LIBNAME)
#	lua test.lua
	shake

clean:
	rm -f $(LIBNAME) $(OBJS)
