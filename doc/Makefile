include ../config.mk

MANDIR1 = $(MANDIR)/man1
MAN1    = $(PROJECT).1

all: man

man: $(MAN1)

%.1: %.1.txt
	@echo build manual page
	@sed -e "s/VERSION/$(VERSION)/g" \
		-e "s/DATE/`date +'%m %Y'`/g" \
		-e "s/PROJECT/$(PROJECT)/g" < $< > $@

install: man
	install -d $(DESTDIR)$(MANDIR1)
	install -m 644 $(MAN1) $(DESTDIR)$(MANDIR1)/$(MAN1)

uninstall:
	$(RM) $(DESTDIR)$(MANDIR1)/$(MAN1)

clean:
	$(RM) *.1

.PHONY: clean
