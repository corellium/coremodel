-include Makefile.local
PREFIX ?=
HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS += -g -Wall -I$(LIBSRC) -Iinclude -Wno-unused-function -DETH_SWITCH_LOGLEVEL=99
LIBSRC ?= ../../
DESTDIR	?= build/

SRCS = \
    eth-switch.c \
    hexdump.c

all: eth-switch

eth-switch: $(SRCS) libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $(SRCS) libcoremodel.a

libcoremodel.a: $(LIBSRC)coremodel.c $(LIBSRC)coremodel.h
	$(HOSTCC) $(CFLAGS) -c -o coremodel.o $(LIBSRC)coremodel.c
	$(HOSTAR) cr $@ coremodel.o
	@rm -f coremodel.o

clean:
	@rm -f eth-switch libcoremodel.a
	@rm -rf

.PHONY: all clean
