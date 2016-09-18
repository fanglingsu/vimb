include config.mk

all: vimb

options:
	@echo "vimb build options:"
	@echo "LIBS    = $(LIBS)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

vimb: $(SUBDIRS:%=%.subdir-all)

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
	# @sed -e "s!VERSION!$(VERSION)!g" \
		# -e "s!PREFIX!$(PREFIX)!g" \
		# -e "s!DATE!`date +'%m %Y'`!g" $(DOCDIR)/vimb.1 > $(MANPREFIX)/man1/vimb.1

uninstall:
	$(RM) $(BINPREFIX)/vimb $(DESTDIR)$(MANDIR)/man1/vimb.1 $(EXTPREFIX)/$(EXTTARGET)

clean: $(SUBDIRS:%=%.subdir-clean)

sandbox:
	@make $(MFLAGS) RUNPREFIX=$(CURDIR)/sandbox/usr PREFIX=/usr DESTDIR=./sandbox install

runsandbox: sandbox
	sandbox/usr/bin/vimb

.PHONY: all options clean install uninstall sandbox sandbox-clean
