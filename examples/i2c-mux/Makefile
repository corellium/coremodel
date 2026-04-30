include ../../Makefile.inc

all: coremodel-i2c-mux

coremodel-i2c-mux: coremodel-i2c-mux.c libcoremodel.a coremodel-i2c.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-i2c-mux libcoremodel.a
	rm -rf *.dSYM .DS_Store
