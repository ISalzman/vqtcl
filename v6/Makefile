all:
	cd lvq && make install && test.lua
	cd tvq && make test
	cd vqx && make all && ./vqmin && ./vqx try.vq

clean:
	cd lvq && make $@
	cd tvq && make $@
	cd vqx && make $@
