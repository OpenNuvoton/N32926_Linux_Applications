.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2/
#INC = $(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib



CC=arm-linux-gcc -O2 -I$(INC) -Wall --static
WEC_LDFLAGS=-L$(LIB) -L$(LIB1) -L.
STRIP=arm-linux-strip

TARGET = rot_demo
SRCS := rot_demo.c device_rot.c device_fb.c
#LIBS = -lgcc
LIBS = 

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS)
	$(STRIP) $(TARGET) 
	@echo "Please copy execution file to storage"
	@echo "Please also copy the pattern to SD card"

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
