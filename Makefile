include config.mk

all: vimb

options:
	@echo "vimb build options:"
	@echo "LIBS      = $(LIBS)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "EXTCFLAGS = $(EXTCFLAGS)"
	@echo "CC        = $(CC)"

$(SRCDIR)/config.h: $(SRCDIR)/config.def.h
	@cp $@ $@.`date +%Y%m%d%H%M%S`
	@cp $< $@

vimb: $(SRCDIR)/config.h $(SUBDIRS:%=%.subdir-all)

%.subdir-all:
	@$(MAKE) $(MFLAGS) -C $*

%.subdir-clean:
	@$(MAKE) $(MFLAGS) -C $* clean

install: vimb
	@# binary
	install -d $(BINPREFIX)
	install -m 755 $(SRCDIR)/vimb $(BINPREFIX)/vimb
	@# extension
	install -d $(EXTPREFIX)
	install -m 644 $(SRCDIR)/webextension/$(EXTTARGET) $(EXTPREFIX)/$(EXTTARGET)
	@# man page
	install -d $(MANPREFIX)/man1
	@sed -e "s!VERSION!$(VERSION)!g" \
		-e "s!PREFIX!$(PREFIX)!g" \
		-e "s!DATE!`date +'%m %Y'`!g" $(DOCDIR)/vimb.1 > $(MANPREFIX)/man1/vimb.1
	@# .desktop file
	install -d $(DOTDESKTOPPREFIX)
	install -m 644 vimb.desktop $(DOTDESKTOPPREFIX)/vimb.desktop

uninstall:
	$(RM) $(BINPREFIX)/vimb $(DESTDIR)$(MANDIR)/man1/vimb.1 $(EXTPREFIX)/$(EXTTARGET)

clean: $(SUBDIRS:%=%.subdir-clean)

sandbox:
	@make $(MFLAGS) RUNPREFIX=$(CURDIR)/sandbox/usr PREFIX=/usr DESTDIR=./sandbox install

runsandbox: sandbox
	sandbox/usr/bin/vimb

.PHONY: all options clean install uninstall sandbox sandbox-clean
