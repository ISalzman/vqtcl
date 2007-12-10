INC = -I../lua/src -I../base -I../lvq/src

vqx: vqx.c
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqx.c -lm && strip $@

vqmin: vqmin.c
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqmin.c -lm && strip $@

vqboot: vqboot.c
	$(CC) -o $@ $(INC) -DNDEBUG -Os vqboot.c -lm && strip $@

all: vqmin vqboot vqx
	
clean:
	rm -f vqmin vqboot vqx
