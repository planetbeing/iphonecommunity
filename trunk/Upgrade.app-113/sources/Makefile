CC=arm-apple-darwin-gcc
LD=arm-apple-darwin-gcc
LDFLAGS=-lobjc -framework CoreFoundation -framework Foundation -framework UIKit -framework LayerKit

UI_OBJS = upgrade.o UpgradeApplication.o
UTIL_OBJS = utilities.o
ALL_OBJS = $(UI_OBJS) $(UTIL_OBJS)

all:	Upgrade writeimage bless

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:	%.m
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@


Upgrade:	$(ALL_OBJS)
		$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)

writeimage:	writeimage.o
		$(LD) -o $@ writeimage.o

bless:	bless.o
		$(LD) -o $@ bless.o

clean:
		rm -f *.o

install:
		cp Upgrade ../resources/
		cp bless ../resources/
		cp writeimage ../resources/
