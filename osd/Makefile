.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib
#INCSYS:=$(ROOT)/arm-linux/sys-include
#INC :=$(ROOT)/arm-linux/include

CC=arm-linux-gcc -O2 -I$(INC) -I$(INCSYS) -static
WEC_LDFLAGS=-L$(LIB) -L$(LIB1)
STRIP=arm-linux-strip

TARGET = osd_demo

SRCS := osd_demo.c

#LIBS= -lc -lgcc -lc

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS)
	$(STRIP) $(TARGET) 
	#cp $(TARGET) ../../initramfs/usr/
	echo "Please also copy input patterns to storage"

clean:
	rm -f *.o 
	rm -f *.x 
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)	
	rm -f *.gdb
