include ../../Makefile.inc

all: coremodel-eth

coremodel-eth: coremodel-eth.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-eth libcoremodel.a
