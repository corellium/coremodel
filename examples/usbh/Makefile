include ../../Makefile.inc

all: coremodel-usbh

coremodel-usbh: coremodel-usbh.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-usbh libcoremodel.a
	rm -rf *.dSYM .DS_Store
