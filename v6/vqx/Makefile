all: vqx
	
vqx: vqx.c
	gcc -o $@ -I../lua/src -I../core -I../lvq/src \
		-DLUA_USE_POSIX -DNDEBUG -Os vqx.c -lm
	strip $@

clean:
	rm -f vqx
