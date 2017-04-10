include config.mk

all: $(SRCDIR).subdir-all

options:
	@echo "vimb build options:"
	@echo "LIBS      = $(LIBS)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "EXTCFLAGS = $(EXTCFLAGS)"
	@echo "CC        = $(CC)"

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
	$(RM) $(BINPREFIX)/vimb $(DESTDIR)$(MANDIR)/man1/vimb.1 $(LIBDIR)/$(EXTTARGET)

clean: $(SRCDIR).subdir-clean

sandbox:
	@make $(MFLAGS) RUNPREFIX=$(CURDIR)/sandbox/usr PREFIX=/usr DESTDIR=./sandbox install

runsandbox: sandbox
	sandbox/usr/bin/vimb

%.subdir-all:
	@$(MAKE) $(MFLAGS) -C $*

%.subdir-clean:
	@$(MAKE) $(MFLAGS) -C $* clean

.PHONY: all options install uninstall clean sandbox runsandbox
