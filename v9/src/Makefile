T = vq
CONFIG = ./config

include $(CONFIG)

SRCS = lua_ext.c datacol.c utils.c
OBJS = lua_ext.o datacol.o utils.o
#LUAS = vq.lua

build: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(LIB_OPTION) -o $(LIBNAME) $(OBJS)

$(OBJS): vq.h defs.h $(SRCS) Makefile $(CONFIG)

install: $(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/vq
	cp $(LIBNAME) $(LUA_LIBDIR)/vq/core.so
	mkdir -p $(LUA_DIR)
#	cp $(LUAS) $(LUA_DIR)

clean:
	rm -f $(LIBNAME) $(OBJS)
