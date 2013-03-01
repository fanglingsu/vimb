#----------------user/install options----------------
VERSION = 0.1.4

PROJECT = vimp
PREFIX  ?= /usr/local/
BINDIR  ?= $(PREFIX)bin/
MANDIR  ?= $(PREFIX)share/man/

#----------------compile options---------------------
LIBS = libsoup-2.4

GTK3LIBS=gtk+-3.0 webkitgtk-3.0
GTK2LIBS=gtk+-2.0 webkit-1.0

ifeq (${GTK}, 3)
ifeq ($(shell pkg-config --exists $(GTK3LIBS) && echo 1), 1) #has gtk3 libs
LIBS += $(GTK3LIBS)
USEGTK3 = 1
else
LIBS += $(GTK2LIBS)
endif
else
LIBS += $(GTK2LIBS)
endif

CFLAGS += $(shell pkg-config --cflags $(LIBS))
CFLAGS += -Wall
CFLAGS += -pipe
CFLAGS += -ansi
CFLAGS += -std=c99
CFLAGS += -pedantic
CFLAGS += -Wmissing-declarations
CFLAGS += -Wmissing-parameter-type
CFLAGS += -Wno-overlength-strings
#CFLAGS += -Wstrict-prototypes

LDFLAGS += $(shell pkg-config --libs $(LIBS)) -lX11 -lXext

# features
CPPFLAGS += -DFEATURE_COOKIE
CPPFLAGS += -DFEATURE_SEARCH_HIGHLIGHT

CPPFLAGS += -DVERSION=\"${VERSION}\" -D_BSD_SOURCE
ifeq ($(USEGTK3), 1)
CPPFLAGS += -DHAS_GTK3
endif


#----------------developer options-------------------
DFLAGS += $(CFLAGS)
DFLAGS += -DDEBUG
DFLAGS += -ggdb
DFLAGS += -g

#----------------end of options----------------------
PP        = m4
OBJ       = $(patsubst %.c, %.o, $(wildcard src/*.c))
DOBJ      = $(patsubst %.c, %.do, $(wildcard src/*.c))
HEAD      = $(wildcard src/*.h)
DEPS      = $(OBJ:%.o=%.d)

TARGET    = $(PROJECT)
DTARGET   = $(TARGET)_dbg
DIST_FILE = $(PROJECT)_$(VERSION).tar.gz

FMOD = 0644

MFLAGS = --no-print-directory
