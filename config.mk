#----------------user/install options----------------
VERSION = 2.0

PROJECT = vimb
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

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

# generate a first char upper case project name
PROJECT_UCFIRST = $(shell echo '${PROJECT}' | awk '{for(i=1;i<=NF;i++){$$i=toupper(substr($$i,1,1))substr($$i,2)}}1')

CPPFLAGS  = -DVERSION=\"${VERSION}\"
CPPFLAGS += -DPROJECT=\"${PROJECT}\" -DPROJECT_UCFIRST=\"${PROJECT_UCFIRST}\"
CPPFLAGS += -D_BSD_SOURCE -D_XOPEN_SOURCE=500
ifeq ($(USEGTK3), 1)
CPPFLAGS += -DHAS_GTK3
endif

# prepare the lib flags used for the linker
LIBFLAGS = $(shell pkg-config --libs $(LIBS)) -lX11 -lXext -lm

# normal compiler flags
CFLAGS  += $(shell pkg-config --cflags $(LIBS))
CFLAGS  += -Wall -pipe -ansi -std=c99 -pedantic
CFLAGS  += -Wmissing-declarations -Wmissing-parameter-type -Wno-overlength-strings
CFLAGS  += ${CPPFLAGS}
LDFLAGS += ${LIBFLAGS}

# compiler flags for the debug target
DFLAGS   += $(CFLAGS) -DDEBUG -ggdb -g
DLDFLAGS += ${LIBFLAGS}

OBJ       = $(patsubst %.c, %.o, $(wildcard src/*.c))
DOBJ      = $(patsubst %.c, %.do, $(wildcard src/*.c))
DEPS      = $(OBJ:%.o=%.d)

TARGET    = $(PROJECT)
DTARGET   = $(TARGET)_dbg
DIST_FILE = $(PROJECT)_$(VERSION).tar.gz
MANDIR1   = $(MANDIR)/man1
MAN1      = $(PROJECT).1
