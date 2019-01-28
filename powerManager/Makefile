#ROOT = /usr/local/arm_linux_4.2
#INC = $(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 = $(ROOT)/arm-linux/lib

#INCPATH	=	-I/usr/gui/mkspecs/qws/linux-arm-g++ -I. -I..
#LIBS	= 	-lts -lm -lrt -ldl -rdynamic -L$(LIB) -L$(LIB1)
LIBS	= 	
DEFINES	=

CC	= 	arm-linux-gcc
CXX	= 	arm-linux-g++
LINK	= 	arm-linux-g++
CFLAGS	= 	-g -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
CXXFLAGS= 	-g -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
LFLAGS	=	-static
STRIP	=	arm-linux-strip
OBJECTS	= 	powerManager.o
TARGET	= 	powerManager

####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

#%.o: %.cpp %.h
#	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

all: 	${TARGET}

$(TARGET):  $(OBJECTS) ${FLLIBS}
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) ${LIBS}
	${STRIP} $(TARGET)

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
