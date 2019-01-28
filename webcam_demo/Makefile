.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2/
#INC :=$(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib

CC=arm-linux-gcc -O2 -g -I$(INC) -static
WEC_LDFLAGS=-L$(LIB) -L$(LIB1)
STRIP=arm-linux-strip

TARGET = webcam
SRCS := cam_lib.c webcam.c fb_lib.c JpegEnc.c
#LIBS = -lgcc
LIBS = 

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static
	$(STRIP) $(TARGET)
	echo "For the demo code, user must make sure that JEPG Engine buffer is enough for use if user select the User Memory Pool Mode."
	echo "JPEG Engien buffer (CONFIG_JPEG_CODEC_BUFFER_SIZE >= BUFFER_SIZE_OTHER_USE + BUFFER_SIZE_WEBCAM_BITSTREAM + BUFFER_COUNT * specified resolution raw data size)"
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
