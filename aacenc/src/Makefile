AR 	= arm-linux-ar 
CC 	= arm-linux-gcc 
LD 	= arm-linux-ld
STRIP	= arm-linux-strip
SHELL 	= sh

OUTPUT_PATH	= ../bin
OUTPUT_NAME	= $(OUTPUT_PATH)/AACRecorder

#ROOT		= /usr/local/arm_linux_4.2
#SYS_INCLUDE	= -I$(ROOT)/arm-linux/include -I$(ROOT)/arm-linux/sys-include -I.
#SYS_LIB		= -L$(ROOT)/lib/gcc/arm-linux/4.2.1 -L$(ROOT)/arm-linux/lib

AS_FLAG		= -O2
C_FLAG		= -O2 -ffunction-sections -fdata-sections -Wall -Wno-strict-aliasing
LD_FLAG		= -static -pthread -Wl,--gc-sections

TARGET_FLAG 	= -mcpu=arm926ej-s

SOURCES		= AACRecorder.c AACRecorder_Agent.c
LSOURCES	= "AACRecorder.c" "AACRecorder_Agent.c"

S_OBJECTS	= $(OUTPUT_PATH)/AACRecorder.o $(OUTPUT_PATH)/AACRecorder_Agent.o
O_OBJECTS	= libnaacenc.a
OBJECTS		= $(S_OBJECTS) $(O_OBJECTS)

LS_OBJECTS	= "$(OUTPUT_PATH)/AACRecorder.o" "$(OUTPUT_PATH)/AACRecorder_Agent.o"
LO_OBJECTS	= "libnaacenc.a"
LOBJECTS	= $(LS_OBJECTS) $(LO_OBJECTS)


all: prebuild $(OUTPUT_NAME) postbuild 


$(OUTPUT_NAME): $(OBJECTS) 
	@echo "Linking... "
	@echo "Creating file $@..."
	@$(CC) -o $@ $(LOBJECTS) $(TARGET_FLAG) $(LD_FLAG) $(SYS_LIB)
	$(STRIP) $(OUTPUT_NAME) 

$(OUTPUT_PATH)/AACRecorder.o: AACRecorder.c Makefile
	@echo "Compiling $<"
	@$(CC) -c -o "$@" "$<" $(TARGET_FLAG) $(C_FLAG) $(SYS_INCLUDE)
$(OUTPUT_PATH)/AACRecorder_Agent.o: AACRecorder_Agent.c Makefile
	@echo "Compiling $<"
	@$(CC) -c -o "$@" "$<" $(TARGET_FLAG) $(C_FLAG) $(SYS_INCLUDE)


clean:
	$(RM) -f $(LS_OBJECTS) 
	$(RM) -f $(OUTPUT_NAME)

postbuild:
	rm -f $(S_OBJECTS)
 
prebuild:
	if [ ! -d "$(OUTPUT_PATH)" ]; then mkdir -p $(OUTPUT_PATH); fi
