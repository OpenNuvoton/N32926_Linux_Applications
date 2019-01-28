.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2/
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib
#INCSYS:=$(ROOT)/arm-linux/sys-include
#INC :=$(ROOT)/arm-linux/include

CC=arm-linux-gcc -O2 -I$(INC) -I$(INCSYS) -static
WEC_LDFLAGS=-L$(LIB) -L$(LIB1)
STRIP=arm-linux-strip

#LIBS= -lc -lgcc -lc -lpthread
TARGET = gprinter_demo
SRCS := g_printer.c 

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static
	$(STRIP) $(TARGET)
#	cp $(TARGET) ../../rootfs-2.6.35/usr/

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
