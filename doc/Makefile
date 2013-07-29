include ../config.mk

MANDIR1 = $(MANDIR)/man1
MAN1    = $(PROJECT).1

install: $(MAN1)
	install -d $(DESTDIR)$(MANDIR1)
	@echo "install -m 644 $(MAN1) $(DESTDIR)$(MANDIR1)/$(MAN1)"
	@sed -e "s/VERSION/$(VERSION)/g" \
		-e "s/DATE/`date +'%m %Y'`/g" \
		-e "s/PROJECT/$(PROJECT)/g" < $(MAN1) > $(DESTDIR)$(MANDIR1)/$(MAN1)
	@chmod 644 $(DESTDIR)$(MANDIR1)/$(MAN1)

uninstall:
	$(RM) $(DESTDIR)$(MANDIR1)/$(MAN1)
