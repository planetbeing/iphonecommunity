BIN_DIR = /usr/bin
CC = arm-apple-darwin-gcc
CFLAGS = -O3

all: clean dmg2img

dmg2img: dmg2img.o base64.o
	$(CC) -o dmg2img dmg2img.o base64.o -L. -lz

dmg2img.o: dmg2img.c dmg2img.h
	$(CC) $(CFLAGS) -c dmg2img.c

base64.o: base64.c base64.h
	$(CC) $(CFLAGS) -c base64.c

install:
	install -m 755 -o root -g root dmg2img $(BIN_DIR)

clean:
	rm -f dmg2img *~ *.o 
