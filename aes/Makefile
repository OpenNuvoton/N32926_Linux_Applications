.SUFFIXES : .x .o .c .s

#ROOT		:=	/usr/local/arm_linux_4.2/
#LIB			:=	$(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1		:=	$(ROOT)/arm-linux/lib
#INC 		:=	$(ROOT)/arm-linux/include

CC			:=	arm-linux-g++ -O2 -I$(INC) -static
WEC_LDFLAGS	:=	-L$(LIB) -L$(LIB1)
STRIP		:=	arm-linux-strip

TARGET		:=	aes_demo
SRCS		:=	main.cpp aes_sw.cpp aes_cryptodev.cpp
#LIBS = -lgcc
LIBS = 


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
