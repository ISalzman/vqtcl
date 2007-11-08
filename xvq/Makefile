CFLAGS = -g -Wall -Wstrict-prototypes -O0 -std=c99

.PHONY: test

xvq.so: xvq.o
	gcc -dynamiclib -fno-common -o $@ xvq.o libtclstub8.5.a

test: xvq.so
	tclkit85aqua xvq.tcl

clean:
	rm -f *.o

distclean: clean
	rm -f *.so tmtags
