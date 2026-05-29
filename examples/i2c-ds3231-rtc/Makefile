include ../../Makefile.inc

all: i2c-ds3231

i2c-ds3231: i2c-ds3231.c libcoremodel.a
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f i2c-ds3231 libcoremodel.a
	rm -rf *.dSYM .DS_Store
