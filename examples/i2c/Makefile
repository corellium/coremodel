include ../../Makefile.inc

all: coremodel-i2c

coremodel-i2c: coremodel-i2c.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-i2c libcoremodel.a
	rm -rf *dSYM .DS_Store
