.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2
#INC = $(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 = $(ROOT)/arm-linux/lib

CC = arm-linux-gcc -O2 -I$(INC) -static
WEC_LDFLAGS = -L$(LIB) -L$(LIB1)
STRIP = arm-linux-strip

TARGET1 = serial_server
TARGET2 = serial_client
SRCS1 = serial_server.c
SRCS2 = serial_client.c
#LIBS = -lgcc
LIBS =

all:
	$(CC) $(WEC_LDFLAGS) $(SRCS1) -o $(TARGET1) $(LIBS) -static
	$(CC) $(WEC_LDFLAGS) $(SRCS2) -o $(TARGET2) $(LIBS) -static
	$(STRIP) $(TARGET1) $(TARGET2)

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET1) $(TARGET2)
	rm -f *.gdb
	rm -f *.bak
