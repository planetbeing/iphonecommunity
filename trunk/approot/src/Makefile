#CC=$(CC)
CXX=/usr/local/arm-apple-darwin/bin/g++
CFLAGS=-fsigned-char -Wall
LDFLAGS=-sysroot=$(HEAVENLY) -Wall -O2
LD=$(CC)

all:	vsu exec_wrapper

vsu:  vsu.c
	$(CC) $(LDFLAGS) -o $@ $^

exec_wrapper: exec_wrapper.c
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o vsu
