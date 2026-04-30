include ../../Makefile.inc

all: coremodel-can

coremodel-can: coremodel-can.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-can libcoremodel.a
	rm -rf *.dSYM .DS_Store
