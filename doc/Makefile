include ../config.mk

MANDIR1 = $(MANDIR)man1/
MAN1    = $(PROJECT).1

all: man

man: $(MAN1)

%.1: %.1.txt
	@echo "${PP} $< > $@"
	@$(PP) -DVERSION=$(VERSION) -DPROJECT=$(PROJECT) $< > $@

install: man
	install -d $(MANDIR1)
	install -m 644 $(MAN1) $(MANDIR1)$(MAN1)

uninstall:
	$(RM) $(MANDIR1)$(MAN1)

clean:
	$(RM) *.1

.PHONY: clean
