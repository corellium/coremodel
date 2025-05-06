-include Makefile.local

HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS = -g -Wall -I$(LIBSRC) -Iinclude -DHUB_LOGCOLOR -DHUB_LOGLEVEL=99 -Wno-unused-function
LIBSRC ?= ../

SRCS = \
    core.c \
    gpio.c \
    can.c \
    coremodel-glue.c \
    rbtree.c \
    nshostport.c

all: coremodel-hub

coremodel-hub: $(SRCS) libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

libcoremodel.a: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) $(CFLAGS) -c -o coremodel.o $(LIBSRC)coremodel.c
	$(HOSTAR) cr $@ coremodel.o
	@rm -f coremodel.o

clean:
	rm -f coremodel-hub libcoremodel.a
