include ../../Makefile.inc

all: coremodel-event

coremodel-event: coremodel-event.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-event libcoremodel.a
