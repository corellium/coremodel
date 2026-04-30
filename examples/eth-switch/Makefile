include ../../Makefile.inc

DESTDIR	?= build/

SRCS = \
    eth-switch.c \
    hexdump.c

all: eth-switch

eth-switch: $(SRCS) libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $(SRCS) libcoremodel.a

clean:
	@rm -f eth-switch libcoremodel.a
	@rm -rf

.PHONY: all clean
