include ../../Makefile.inc

all: spi-ds3234

spi-ds3234: spi-ds3234.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f spi-ds3234 libcoremodel.a
	rm -rf *.dSYM .DS_Store
