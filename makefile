
PREFIX=arm-linux-gnueabihf-

INCLUDES=-I.

CC=$(PREFIX)gcc
CP=cp
MV=mv
RM=rm -f

oled.a:	oled.o
	$(CC) -o oled.a oled.o
	$(MV) ./oled.a ~/tftp/nfsroot/home/nemo/.
	sync

oled.o:	oled.c oled.h font8x8.h
	$(CC) -c -I$(INCLUDES) oled.c -o oled.o


clean:
	$(RM) *.o
