BASEDIR=../..
include $(BASEDIR)/config.mk

JSFILES = $(wildcard *.js)

all: scripts.h

clean:
	$(RM) scripts.h

scripts.h: $(JSFILES)
	@echo "create $@ from *.js"
	@echo > $@
	@for file in $(JSFILES); do \
		./js2h.sh $$file >> $@; \
	done

.PHONY: all clean

