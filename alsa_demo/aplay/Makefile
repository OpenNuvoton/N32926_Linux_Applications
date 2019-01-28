.SUFFIXES : .x .o .c .s
#ROOT = /usr/local/arm_linux_4.2/
LIB = ../lib
INC =../include
CC=arm-linux-gcc -O2 -I$(INC) -static
LDFLAGS=-L$(LIB)
STRIP=arm-linux-strip

TARGET = aplay
SRCS := aplay.c 
LIBS= -lasound -lm -ldl -lpthread

all: 
	$(CC) $(LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static
	$(STRIP) $(TARGET) 
	ln -s aplay arecord

clean:
	rm -f *.o 
	rm -f aplay arecord
