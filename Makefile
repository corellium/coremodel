-include Makefile.local

HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS = -g -Wall -I$(LIBSRC)
LIBSRC ?= ./

all: libcoremodel.so libcoremodel.a

libcoremodel.so: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) -shared -fPIC $(CFLAGS) -o $@ $(LIBSRC)coremodel.c
	@chmod a-x $@
libcoremodel.a: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) $(CFLAGS) -c -o coremodel.o $(LIBSRC)coremodel.c
	$(HOSTAR) cr $@ coremodel.o
	@rm -f coremodel.o

clean:
	rm -f libcoremodel.so libcoremodel.a
