include ../../Makefile.inc

all: coremodel-list

coremodel-list: coremodel-list.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-list libcoremodel.a
	rm -rf *.dSYM .DS_Store
