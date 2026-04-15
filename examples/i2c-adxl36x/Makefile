-include Makefile.local

HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS = -g -Wall -I$(LIBSRC) -ldl -Wl,--no-as-needed -lm -lpthread
LIBSRC ?= ../../

all: coremodel-adxl36x

coremodel-adxl36x: coremodel-adxl36x.c libcoremodel.a adxl36x_registers.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

libcoremodel.a: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) $(CFLAGS) -c -o coremodel.o $(LIBSRC)coremodel.c
	$(HOSTAR) cr $@ coremodel.o
	@rm -f coremodel.o

clean:
	rm -f coremodel-adxl36x libcoremodel.a