include config.mk

all: vimb

options:
	@echo "vimb build options:"
	@echo "LIBS    = $(LIBS)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

install: vimb
	@# binary
	install -d $(BINPREFIX)
	install -m 755 $(SRCDIR)/vimb $(BINPREFIX)/vimb
	@# extension
	install -d $(EXTPREFIX)
	install -m 644 $(SRCDIR)/webextension/$(EXTTARGET) $(EXTPREFIX)/$(EXTTARGET)

uninstall:
	$(RM) $(BINPREFIX)/vimb $(EXTPREFIX)/$(EXTTARGET)

vimb: $(SUBDIRS:%=%.subdir-all)

%.subdir-all:
	$(MAKE) $(MFLAGS) -C $*

%.subdir-clean:
	$(MAKE) $(MFLAGS) -C $* clean

clean: $(SUBDIRS:%=%.subdir-clean)

runsandbox: sandbox
	sandbox/usr/bin/vimb

sandbox:
	make RUNPREFIX=$(CURDIR)/sandbox/usr PREFIX=/usr DESTDIR=./sandbox install

.PHONY: all options clean install uninstall sandbox
