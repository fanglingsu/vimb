VERSION = dev-3.0

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

# setup general used CFLAGS
CFLAGS     += -std=c99 -pipe -Wall
#CFLAGS     += -pedantic
CFLAGS     += -DVERSION=\"${VERSION}\" -DEXTENSIONDIR=\"${EXTENSIONDIR}\"
CFLAGS     += -DPROJECT=\"vimb\" -DPROJECT_UCFIRST=\"Vimb\"
CFLAGS     += -D_XOPEN_SOURCE=500
CFLAGS     += -DGSEAL_ENABLE
CFLAGS     += -DGTK_DISABLE_SINGLE_INCLUDES
CFLAGS     += -DGDK_DISABLE_DEPRECATED

# flags used to build webextension
EXTTARGET   = webext_main.so
EXTCFLAGS   = ${CFLAGS} -fPIC $(shell pkg-config --cflags webkit2gtk-4.0)
EXTLDFLAGS  = $(shell pkg-config --libs webkit2gtk-4.0) -shared

# flags used for the main application
CFLAGS     += $(shell pkg-config --cflags $(LIBS))
LDFLAGS    += $(shell pkg-config --libs $(LIBS))

# flags for make
MFLAGS      = --no-print-directory
