include ../../Makefile.inc

all: coremodel-spi-mux

coremodel-spi-mux: coremodel-spi-mux.c libcoremodel.a coremodel-spi.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-spi-mux libcoremodel.a
	rm -rf *.dSYM .DS_Store
