VERSION = dev-3.0

PREFIX           ?= /usr/local
BINPREFIX        := $(DESTDIR)$(PREFIX)/bin
MANPREFIX        := $(DESTDIR)$(PREFIX)/share/man
EXAMPLEPREFIX    := $(DESTDIR)$(PREFIX)/share/vimb/example
DOTDESKTOPPREFIX := $(DESTDIR)$(PREFIX)/share/applications
RUNPREFIX        := $(PREFIX)
EXTPREFIX        := $(RUNPREFIX)/lib/vimb

# define some directories
SRCDIR  = src
DOCDIR  = doc
SUBDIRS = $(SRCDIR)/scripts $(SRCDIR)/webextension $(SRCDIR)

# used libs
LIBS = gtk+-3.0 'webkit2gtk-4.0 >= 2.3.5'

# setup general used CFLAGS
CFLAGS   += -std=c99 -pipe -Wall
#CPPFLAGS += -pedantic
CPPFLAGS += -DVERSION=\"${VERSION}\" -DEXTPREFIX=\"${EXTPREFIX}\"
CPPFLAGS += -DPROJECT=\"vimb\" -DPROJECT_UCFIRST=\"Vimb\"
CPPFLAGS += -D_XOPEN_SOURCE=500
CPPFLAGS += -DGSEAL_ENABLE
CPPFLAGS += -DGTK_DISABLE_SINGLE_INCLUDES
CPPFLAGS += -DGDK_DISABLE_DEPRECATED

# flags used to build webextension
EXTTARGET   = webext_main.so
EXTCFLAGS   = ${CFLAGS} -fPIC $(shell pkg-config --cflags webkit2gtk-4.0)
EXTCFLAGS  += $(CPPFLAGS)
EXTLDFLAGS  = $(shell pkg-config --libs webkit2gtk-4.0) -shared

# flags used for the main application
CFLAGS     += $(shell pkg-config --cflags $(LIBS))
CFLAGS     += ${CPPFLAGS}
LDFLAGS    += $(shell pkg-config --libs $(LIBS))
MFLAGS      = --no-print-directory
