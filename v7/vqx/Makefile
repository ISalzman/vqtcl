INC = -I../lvq -I../lua/src

vqx: vqx.c ../lvq/*.[ch]
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqx.c -lm && strip $@

debug:
	$(CC) -o vqx_g $(INC) -DLUA_USE_APICHECK -g -O0 vqx.c -lm
	
test: vqx
	./vqx ../lvq/test.lua
	
clean:
	rm -rf vqx vqx_g*
