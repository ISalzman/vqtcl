INC = -I../lvq -I../lua/src

vqx: vqx.c
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqx.c -lm && strip $@

test: vqx
	./vqx ../lvq/test.lua
	
clean:
	rm -f vqx
