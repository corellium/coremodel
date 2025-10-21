-include Makefile.local

HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS = -g -Wall -I$(LIBSRC)
LIBSRC ?= ../../

all: can-cr3020 can-cansend

can-cr3020: can-cr3020.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

can-cansend: can-cansend.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

libcoremodel.a: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) $(CFLAGS) -c -o coremodel.o $(LIBSRC)coremodel.c
	$(HOSTAR) cr $@ coremodel.o
	@rm -f coremodel.o

clean:
	rm -f can-cr3020 can-cansend libcoremodel.a
	rm -rf *.dSYM .DS_Store
