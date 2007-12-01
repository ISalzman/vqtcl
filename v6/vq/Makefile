all: vq
	
vq: vq.c
	gcc -o $@ -I../lua/src -I../lvq/src -DLUA_USE_POSIX -Os vq.c
	strip $@

clean:
	rm -f vq