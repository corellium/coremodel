include ../../Makefile.inc

all: can-cr3020 can-cansend

can-cr3020: can-cr3020.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

can-cansend: can-cansend.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f can-cr3020 can-cansend libcoremodel.a
	rm -rf *.dSYM .DS_Store
