CC=arm-apple-darwin-gcc
OBJS= util.o \
      vfdecrypt.o

vfdecrypt: util.o vfdecrypt.o
	$(CC) $(OBJS) -o $@ -lcrypto

clean:
	rm -f $(OBJS) vfdecrypt
