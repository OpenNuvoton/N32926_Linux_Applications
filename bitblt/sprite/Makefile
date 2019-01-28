.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2
#INC = $(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 = $(ROOT)/arm-linux/lib

CC = arm-linux-gcc -O2 -I$(INC) -static -I../include
WEC_LDFLAGS = -L$(LIB) -L$(LIB1)
STRIP = arm-linux-strip

TARGET = sprite
SRCS = main.c gfx_demo_sprite.c ../lib/gfx.c
LIBS = -lm

all:
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static
	$(STRIP) $(TARGET)

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)	
	rm -f *.gdb
	rm -f *.bak
