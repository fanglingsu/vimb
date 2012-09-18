#----------------user/install options----------------
VERSION = 0.1.0

PROJECT = vimprobable
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin/
MANDIR ?= $(PREFIX)/share/man/

#----------------compile options---------------------
LIBS = gtk+-2.0 webkit-1.0

CFLAGS += $(shell pkg-config --cflags $(LIBS))
CFLAGS += -Wall
CFLAGS += -pipe
CFLAGS += -ansi
CFLAGS += -std=c99
CFLAGS += -pedantic
CFLAGS += -Wmissing-declarations
CFLAGS += -Wmissing-parameter-type
#CFLAGS += -Wstrict-prototypes

LDFLAGS += $(shell pkg-config --libs $(LIBS)) -lX11 -lXext

CPPFLAGS += -DPROJECT=\"$(PROJECT)\"
CPPFLAGS += -DVERSION=\"${VERSION}\"

#----------------developer options-------------------
DFLAGS += $(CFLAGS)
DFLAGS += -DDEBUG
DFLAGS += -ggdb
DFLAGS += -g

#----------------end of options----------------------
OBJ       = $(patsubst %.c, %.o, $(wildcard src/*.c))
DOBJ      = $(patsubst %.c, %.do, $(wildcard src/*.c))
HEAD      = $(wildcard *.h)
DEPS      = $(OBJ:%.o=%.d)

TARGET    = $(PROJECT)
DTARGET   = $(TARGET)_dbg
DIST_FILE = $(PROJECT)_$(VERSION).tar.gz

FMOD = 0644

MFLAGS = --no-print-directory
