all: vqx
	
vqx: vqx.c
	gcc -o $@ -I../lua/src -I../lvq/src -DLUA_USE_POSIX -Os vqx.c
	strip $@

clean:
	rm -f vqx
