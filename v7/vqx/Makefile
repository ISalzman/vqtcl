INC = -I../src -I../lua/src

vqx: vqx.c ../src/*.[ch]
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqx.c -lm && strip $@

debug:
	$(CC) -o vqx_g $(INC) -DLUA_USE_APICHECK -g -O0 vqx.c -lm
	
test: vqx
	./vqx test.vq
	
clean:
	rm -rf vqx vqx_g*
