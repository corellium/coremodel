include ../../Makefile.inc

all: coremodel-gpio coremodel-blinky

coremodel-gpio: coremodel-gpio.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

coremodel-blinky: coremodel-blinky.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-gpio libcoremodel.a coremodel-blinky
	rm -rf *dSYM .DS_Store
