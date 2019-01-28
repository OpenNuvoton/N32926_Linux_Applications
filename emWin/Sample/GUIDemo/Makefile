.SUFFIXES : .x .o .c .s

CC=arm-linux-gcc
LINK=$(CC)
STRIP=arm-linux-strip
CFLAGS=-MD -O2
LDFLAGS=-L../../Lib -lNUemWin -lm -L./tslib -lts -ldl -pthread -static

APPDIR=.
CONFHEADER=../../Config
GUIHEADER=../../Include
CONFDIR=$(CONFHEADER)
TSLIBDIR=tslib
TSLIBHEADER=$(TSLIBDIR)
GUIX=GUI_X
LCDCONF=LCDConf

BINARY=GUIDemo

APPSOURCES := $(shell find $(APPDIR) -name '*.c')
APPOBJECTS := $(addprefix ./,$(APPSOURCES:%.c=%.o))
APPDFILES := $(addprefix ./,$(APPSOURCES:%.c=%.d))

RM=rm -rf

all: $(BINARY)

$(APPDIR)/%.o: $(APPDIR)/%.c
	$(CC) $(CFLAGS) -I. -I$(CONFHEADER) -I$(GUIHEADER) -I$(TSLIBHEADER) -c $< -o $@

$(CONFDIR)/$(GUIX).o: $(CONFDIR)/$(GUIX).c
	$(CC) $(CFLAGS) -I. -I$(CONFHEADER) -I$(GUIHEADER) -c $< -o $@

$(CONFDIR)/$(LCDCONF).o: $(CONFDIR)/$(LCDCONF).c
	$(CC) $(CFLAGS) -I. -I$(CONFHEADER) -I$(GUIHEADER) -c $< -o $@

$(BINARY): $(APPOBJECTS) $(CONFDIR)/$(GUIX).o $(CONFDIR)/$(LCDCONF).o
	$(LINK) $(APPOBJECTS) $(CONFDIR)/$(GUIX).o $(CONFDIR)/$(LCDCONF).o $(LDFLAGS) -o $(BINARY)
	$(STRIP) $(BINARY)

clean:
	$(RM) $(BINARY) $(APPOBJECTS) $(APPDFILES) $(CONFDIR)/$(GUIX).o $(CONFDIR)/$(GUIX).d $(CONFDIR)/$(LCDCONF).o $(CONFDIR)/$(LCDCONF).d
