include ../../Makefile.inc

all: coremodel-spi

coremodel-spi: coremodel-spi.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-spi libcoremodel.a
	rm -rf *.dSYM .DS_Store
