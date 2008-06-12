T = vq
CONFIG = ./config

include $(CONFIG)

DEPS = vq.c vlerq.h defs.h ops.c nulls.c mutable.c file.c buffer.c emit.c \
	sort.c group.c hash.c mapped.c core.c
OBJS = vq.o
LUAS = vq.lua

$(LIBNAME): $(OBJS)
	$(CC) $(LIB_OPTION) -o $(LIBNAME) $(OBJS)

vq.o: $(DEPS) Makefile $(CONFIG)

install: $(LIBNAME)
	mkdir -p $(LUA_LIBDIR)/vq
	cp $(LIBNAME) $(LUA_LIBDIR)/vq/core.so
	mkdir -p $(LUA_DIR)
	cp $(LUAS) $(LUA_DIR)

test: $(LIBNAME)
#	lua test.lua
	shake

clean:
	rm -f $(LIBNAME) $(OBJS)
