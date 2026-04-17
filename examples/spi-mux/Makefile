-include Makefile.local

HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS = -g -Wall -I$(LIBSRC) -ldl -lm -lpthread
OS := $(shell uname -s)
ifeq ($(OS),Darwin)
	CFLAGS +=
else
	CFLAGS += -Wl,--no-as-needed
endif
LIBSRC ?= ../../

all: coremodel-spi-mux

coremodel-spi-mux: coremodel-spi-mux.c libcoremodel.a coremodel-spi.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

libcoremodel.a: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) $(CFLAGS) -c -o coremodel.o $(LIBSRC)coremodel.c
	$(HOSTAR) cr $@ coremodel.o
	@rm -f coremodel.o

clean:
	rm -f coremodel-spi-mux libcoremodel.a
	rm -rf *.dSYM .DS_Store
