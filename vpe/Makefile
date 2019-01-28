.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2/
#INC :=$(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib

CC=arm-linux-gcc -O2 -I$(INC) -static
WEC_LDFLAGS=-L$(LIB) -L$(LIB1)
STRIP=arm-linux-strip

TARGET = vpe_demo
SRCS := vpe_demo.c
#LIBS = -lgcc
LIBS = 

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static
	$(STRIP) $(TARGET) 
#	cp $(TARGET) ../../rootfs/usr/
#	echo "Please copy execution file and source patterns to sdcard. And execute it in sdcard"

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
