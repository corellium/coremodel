include ../../Makefile.inc

all: coremodel-uart

coremodel-uart: coremodel-uart.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-uart libcoremodel.a
	rm -rf *.dSYM .DS_Store
