all: vq
	
vq: vq.c
	gcc -o $@ -I../lua/src -I../lvq/src vq.c
	strip $@

clean:
	rm -f vq