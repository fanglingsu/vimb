VERSION = 3.1.0

ifneq ($(V),1)
Q := @
endif

PREFIX           ?= /usr/local
BINPREFIX        := $(DESTDIR)$(PREFIX)/bin
MANPREFIX        := $(DESTDIR)$(PREFIX)/share/man
EXAMPLEPREFIX    := $(DESTDIR)$(PREFIX)/share/vimb/example
DOTDESKTOPPREFIX := $(DESTDIR)$(PREFIX)/share/applications
LIBDIR           := $(DESTDIR)$(PREFIX)/lib/vimb
RUNPREFIX        := $(PREFIX)
EXTENSIONDIR     := $(RUNPREFIX)/lib/vimb

# define some directories
SRCDIR  = src
DOCDIR  = doc

# used libs
LIBS = gtk+-3.0 'webkit2gtk-4.0 >= 2.3.5'

COMMIT := $(shell git describe --tags --always 2> /dev/null || echo "unknown")

# setup general used CFLAGS
CFLAGS   += -std=c99 -pipe -Wall

#CPPFLAGS += -pedantic
CPPFLAGS += -DVERSION=\"${VERSION}\" -DEXTENSIONDIR=\"${EXTENSIONDIR}\" -DCOMMIT=\"$(COMMIT)\"
CPPFLAGS += -DPROJECT=\"vimb\" -DPROJECT_UCFIRST=\"Vimb\"
CPPFLAGS += -D_XOPEN_SOURCE=500
CPPFLAGS += -D__BSD_VISIBLE
CPPFLAGS += -DGSEAL_ENABLE
CPPFLAGS += -DGTK_DISABLE_SINGLE_INCLUDES
CPPFLAGS += -DGDK_DISABLE_DEPRECATED

# flags used to build webextension
EXTTARGET   = webext_main.so
EXT_CFLAGS  = -fPIC $(shell pkg-config --cflags webkit2gtk-4.0) $(CPPFLAGS) $(CFLAGS)
EXT_LDFLAGS = $(shell pkg-config --libs webkit2gtk-4.0) -shared $(LDFLAGS)

# flags used for the main application
VIMB_CFLAGS  = $(shell pkg-config --cflags $(LIBS)) $(CPPFLAGS) $(CFLAGS) 
VIMB_LDFLAGS = $(shell pkg-config --libs $(LIBS)) $(LDFLAGS)
