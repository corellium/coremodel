include ../../Makefile.inc

all: coremodel-adxl36x

coremodel-adxl36x: coremodel-adxl36x.c libcoremodel.a adxl36x_registers.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f coremodel-adxl36x libcoremodel.a
	rm -rf *.dSYM .DS_Store