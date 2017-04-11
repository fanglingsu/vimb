#----------------user/install options----------------
VERSION = 2.12

PROJECT     = vimb
PREFIX     ?= /usr/local
BINDIR     ?= $(PREFIX)/bin
DATADIR    ?= $(PREFIX)/share
MANDIR     ?= $(DATADIR)/man
EXAMPLEDIR ?= $(DATADIR)/$(PROJECT)/examples

#----------------compile options---------------------

VERBOSE ?= 0

LIBS = libsoup-2.4

GTK3LIBS=gtk+-3.0 webkitgtk-3.0
GTK2LIBS=gtk+-2.0 webkit-1.0

ifeq (${GTK}, 3)
ifeq ($(shell pkg-config --exists $(GTK3LIBS) && echo 1), 1) #has gtk3 libs
LIBS += $(GTK3LIBS)
USEGTK3 = 1
else
LIBS += $(GTK2LIBS)
$(warning Cannot find gtk3-libs, falling back to gtk2)
endif
else
LIBS += $(GTK2LIBS)
endif

# generate a first char upper case project name
PROJECT_UCFIRST = $(shell echo '${PROJECT}' | awk '{for(i=1;i<=NF;i++){$$i=toupper(substr($$i,1,1))substr($$i,2)}}1')

CPPFLAGS  = -DVERSION=\"${VERSION}\"
CPPFLAGS += -DPROJECT=\"${PROJECT}\" -DPROJECT_UCFIRST=\"${PROJECT_UCFIRST}\"
CPPFLAGS += -D_XOPEN_SOURCE=500
CPPFLAGS += -D_POSIX_SOURCE
ifeq ($(USEGTK3), 1)
CPPFLAGS += -DHAS_GTK3
CPPFLAGS += -DGSEAL_ENABLE
CPPFLAGS += -DGTK_DISABLE_SINGLE_INCLUDES
CPPFLAGS += -DGTK_DISABLE_DEPRECATED
CPPFLAGS += -DGDK_DISABLE_DEPRECATED
endif

# prepare the lib flags used for the linker
LIBFLAGS = $(shell pkg-config --libs $(LIBS))

# some compiler flags in case CFLAGS is not set by user
# -Wno-typedef-redefinition to avoid redifinition warnings caused by glib
CFLAGS  ?= -Wall -pipe -Wno-overlength-strings -Werror=format-security -Wno-typedef-redefinition
# normal compiler flags
CFLAGS  += $(shell pkg-config --cflags $(LIBS))
CFLAGS  += -std=c99
CFLAGS  += ${CPPFLAGS}
LDFLAGS += ${LIBFLAGS}

TARGET    = $(PROJECT)
LIBTARGET = lib$(PROJECT).so
DIST_FILE = $(PROJECT)_$(VERSION).tar.gz
MAN1      = $(PROJECT).1

MFLAGS   ?= --no-print-directory
