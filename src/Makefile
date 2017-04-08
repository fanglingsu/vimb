BASEDIR=..
include $(BASEDIR)/config.mk

SUBDIRS = webextension
OBJ     = $(patsubst %.c, %.o, $(wildcard *.c))
JSFILES = $(wildcard scripts/*.js)

all: vimb $(SUBDIRS:%=%.subdir-all)

clean: $(SUBDIRS:%=%.subdir-clean)
	$(RM) vimb *.o scripts/scripts.h

vimb: $(OBJ)
	$(Q)echo "${CC} $@"
	$(Q)$(CC) $(LDFLAGS) $(OBJ) -o $@

$(OBJ): config.h $(BASEDIR)/config.mk scripts/scripts.h

-include $(OBJ:.o=.d)

config.h:
	$(Q)echo create $@ from config.def.h
	$(Q)cp config.def.h $@

scripts/scripts.h: $(JSFILES)
	$(Q)$(RM) $@
	$(Q)echo "create $@ from *.js"
	$(Q)for file in $(JSFILES); do \
		./scripts/js2h.sh $$file >> $@; \
	done

%.o: %.c
	$(Q)echo "${CC} $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

%.subdir-all: config.h
	$(Q)$(MAKE) $(MFLAGS) -C $*

%.subdir-clean:
	$(Q)$(MAKE) $(MFLAGS) -C $* clean

.PHONY: all clean
