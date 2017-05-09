BASEDIR=..
include $(BASEDIR)/config.mk

SUBDIRS  = webextension
OBJ      = $(patsubst %.c, %.o, $(wildcard *.c))
JSFILES  = $(wildcard scripts/*.js)
CSSFILES = $(wildcard scripts/*.css)

all: vimb $(SUBDIRS:%=%.subdir-all)

clean: $(SUBDIRS:%=%.subdir-clean)
	$(RM) vimb *.o scripts/scripts.h

vimb: $(OBJ)
	@echo "${CC} $@"
	$(Q)$(CC) $(OBJ) $(LDFLAGS) -o $@

$(OBJ): config.h $(BASEDIR)/config.mk scripts/scripts.h

-include $(OBJ:.o=.d)

config.h:
	@echo create $@ from config.def.h
	$(Q)cp config.def.h $@

scripts/scripts.h: $(JSFILES) $(CSSFILES)
	$(Q)$(RM) $@
	@echo "create $@ from *.{css,js}"
	$(Q)for file in $(JSFILES) $(CSSFILES); do \
		./scripts/js2h.sh $$file >> $@; \
	done

%.o: %.c
	@echo "${CC} $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

%.subdir-all: config.h
	$(Q)$(MAKE) -C $*

%.subdir-clean:
	$(Q)$(MAKE) -C $* clean

.PHONY: all clean
