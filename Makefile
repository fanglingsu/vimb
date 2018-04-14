include config.mk

all: src.subdir-all

options:
	@echo "vimb build options:"
	@echo "LIBS      = $(LIBS)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "EXTCFLAGS = $(EXTCFLAGS)"
	@echo "CC        = $(CC)"

install: src.subdir-all
	@# binary
	install -d $(BINPREFIX)
	install -m 755 src/vimb $(BINPREFIX)/vimb
	@# extension
	install -d $(LIBDIR)
	install -m 644 src/webextension/$(EXTTARGET) $(LIBDIR)/$(EXTTARGET)
	@# man page
	install -d $(MANPREFIX)/man1
	@sed -e "s!VERSION!$(VERSION)!g" \
		-e "s!PREFIX!$(PREFIX)!g" \
		-e "s!DATE!`date +'%m %Y'`!g" $(DOCDIR)/vimb.1 > $(MANPREFIX)/man1/vimb.1
	@# .desktop file
	install -d $(DOTDESKTOPPREFIX)
	install -m 644 vimb.desktop $(DOTDESKTOPPREFIX)/vimb.desktop

uninstall:
	$(RM) $(BINPREFIX)/vimb
	$(RM) $(DESTDIR)$(MANDIR)/man1/vimb.1
	$(RM) $(LIBDIR)/$(EXTTARGET)
	$(RM) $(DOTDESKTOPPREFIX)/vimb.desktop

clean: src.subdir-clean

sandbox:
	$(Q)$(MAKE) RUNPREFIX=$(CURDIR)/sandbox/usr PREFIX=/usr DESTDIR=./sandbox install

runsandbox: sandbox
	sandbox/usr/bin/vimb

test:
	$(MAKE) -C src vimb.so
	$(MAKE) -C tests

%.subdir-all:
	$(Q)$(MAKE) -C $*

%.subdir-clean:
	$(Q)$(MAKE) -C $* clean

.PHONY: all options install uninstall clean sandbox runsandbox
