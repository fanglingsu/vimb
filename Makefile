include config.mk

all: $(SRCDIR).subdir-all

options:
	@echo "vimb build options:"
	@echo "LIBS        = $(LIBS)"
	@echo "CFLAGS      = $(VIMB_CFLAGS)"
	@echo "LDFLAGS     = $(VIIMB_LDFLAGS)"
	@echo "EXT_CFLAGS  = $(EXT_CFLAGS)"
	@echo "EXT_LDFLAGS = $(EXT_LDFLAGS)"
	@echo "CC          = $(CC)"

install: $(SRCDIR).subdir-all
	@# binary
	install -d $(BINPREFIX)
	install -m 755 $(SRCDIR)/vimb $(BINPREFIX)/vimb
	@# extension
	install -d $(LIBDIR)
	install -m 644 $(SRCDIR)/webextension/$(EXTTARGET) $(LIBDIR)/$(EXTTARGET)
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

clean: $(SRCDIR).subdir-clean

sandbox:
	$(Q)$(MAKE) RUNPREFIX=$(CURDIR)/sandbox/usr PREFIX=/usr DESTDIR=./sandbox install

runsandbox: sandbox
	sandbox/usr/bin/vimb

%.subdir-all:
	$(Q)$(MAKE) -C $*

%.subdir-clean:
	$(Q)$(MAKE) -C $* clean

.PHONY: all options install uninstall clean sandbox runsandbox
