.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2/
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib
LIBJPEG =./libjpeg
#INC :=$(ROOT)/arm-linux/include
INCJPEG =./libjpeg

CC=arm-linux-gcc -O2 -I$(INCJPEG) -I$(INC) -static
WEC_LDFLAGS=-L$(LIBJPEG) -L$(LIB) -L$(LIB1)
STRIP=arm-linux-strip

TARGET = ijg_jpeg_demo
SRCS := ijg_jpeg_demo.c 
LIBS= -ljpeg

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
