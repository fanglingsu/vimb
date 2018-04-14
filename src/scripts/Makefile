include ../../config.mk

JSFILES  = $(wildcard *.js)
CSSFILES = $(wildcard *.css)

all: scripts.h

clean:
	$(RM) scripts.h

scripts.h: $(JSFILES) $(CSSFILES)
	$(Q)$(RM) $@
	@echo "create $@ from *.{css,js}"
	$(Q)for file in $(JSFILES) $(CSSFILES); do \
		./js2h.sh $$file >> $@; \
	done

.PHONY: all clean
