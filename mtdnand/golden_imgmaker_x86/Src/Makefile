.SUFFIXES : .x .o .c .s

CC=gcc -O3 -m32 -static
TARGET = GoldenImgMaker
LIBS=-lm
SRCS :=  main.c bch_encoder_verify.c ProcessOptionalIni.c info_elnec.c

all: 
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS)
	strip $(TARGET)

clean:
	rm -f *.o 
	rm -f *.x 
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)
	rm -f *.gdb
