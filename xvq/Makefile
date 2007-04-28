CFLAGS = -g -Wall -Wstrict-prototypes -O0 -std=c99

.PHONY: test

test: xvq.so
	tclkit85aqua xvq.tcl

xvq.so: xvq.o
	gcc -dynamiclib -fno-common -o $@ xvq.o libtclstub8.5.a

clean:
	rm -f *.o

distclean: clean
	rm -f *.so tmtags
