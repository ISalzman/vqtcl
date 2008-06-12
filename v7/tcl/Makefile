ifeq ($(ARCHSUFFIX),)
	dll = .so
	dyn = -shared
	# find Tcl headers on Ubuntu Linux:
	CFLAGS += -I/usr/include/tcl8.4
	CFLAGS += -fPIC
endif
ifeq ($(ARCHSUFFIX),.macosx)
	dll = .dylib
	dyn = -dynamiclib -fno-common
	#CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk
endif
ifeq ($(ARCHSUFFIX),.win32)
	CC = gcc
	# find Tcl headers when cross-compiling on macosx via xmingw32.sh:
	CFLAGS += -I/System/Library/Frameworks/Tcl.framework/Headers
	CFLAGS += -fomit-frame-pointer -Wno-long-long
	dll = .dll
	dyn = -shared
endif

CFLAGS += -Wall -pedantic -Wstrict-prototypes -Wno-unknown-pragmas -ansi
CFLAGS += -I../src -I../lua/src
CFLAGS += -DDEBUG -g -O0

TARGET = libtvq$(dll)

OBJ = tvq.o luall.o
		
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(dyn) -o $(TARGET) $(OBJ)

luall.o: ../src/luall.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	tclkit test.tcl

clean:
	rm -f libtvq.* *.o
	
.PHONY: all test clean
