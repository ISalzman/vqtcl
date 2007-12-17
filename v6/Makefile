all:
	cd lvq && make install && tests/test.lua
	cd tvq && make test
	cd vqx && make && ./vqx try.vq

clean:
	cd lvq && make $@
	cd tvq && make $@
	cd vqx && make $@
