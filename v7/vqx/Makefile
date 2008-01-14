INC = -I../lvq -I../lua/src

vqx: vqx.c ../lvq/*.[ch]
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqx.c -lm && strip $@

debug:
	$(CC) -o vqx_g $(INC) -g -O0 vqx.c -lm
	
test: vqx
	./vqx ../lvq/test.lua
	
clean:
	rm -f vqx vqx_g
