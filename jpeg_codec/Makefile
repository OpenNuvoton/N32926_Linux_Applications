.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2/
#INC :=$(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 =$(ROOT)/arm-linux/lib

CC=arm-linux-gcc -O2 -g -I$(INC) -static
WEC_LDFLAGS=-L$(LIB) -L$(LIB1)
STRIP=arm-linux-strip

TARGET = jpeg_demo
SRCS := jpeg_demo.c exif.c
#LIBS = -lgcc
LIBS = 

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static
	$(STRIP) $(TARGET)
	echo "Please also copy input pattern for Decode input wait test to storage"	
	echo "Test 0: Jpeg Decode Input & Output Test -> jpegIOPW.jpg"
	echo "Test 1: Jpeg Normal Encode Test -> jpegEnc.dat"
	echo "Test 2: Jpeg Normal Decode Test -> jpegDec.jpg"
	echo "Test 3: Jpeg Decode Downscale & Stride Test -> jpegDec.jpg"
	echo "Test 4: Jpeg Encode Upscale Test -> jpegEnc.dat"		
	echo "Test 5: Jpeg [Decode Input Wait] Normal Decode Test -> jpegIPW.jpg"
	echo "Test 6: Jpeg [Decode Input Wait] Decode Downscale & Stride Test -> jpegIPW.jpg"
	echo "Test 7: Jpeg Decode Output Wait Test -> jpegOPW.jpg"	
	echo "Test 8: Jpeg Window Decode Test -> jpegWIN.jpg"
	echo "Test 9: Jpeg Encode File with Reserved size for Application -> jpegEnc.dat"
	echo "Test D: Jpeg Decode Downscale & Stride Test (To FB Buffer) -> jpegDec.jpg"	
	echo "Test E: Jpeg Window Decode & Downscale Test (To FB Buffer) -> thumb0.jpg~thumb3.jpg"
  
	echo "Decode Without Input Wait : jpegDec.jpg"
	echo "Encode Normal / Upscale : jpegEnc.dat (VGA)"
	echo "Decode With Input Wait : jpegIPW.jpg"
	echo "Decode With Output Wait : jpegOPW.jpg"
	echo "Decode With Input & Output Wait : jpegIOPW.jpg"
	echo "Window Decode : jpegWIN.jpg"

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
