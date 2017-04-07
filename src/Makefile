BASEDIR=..
include $(BASEDIR)/config.mk

SUBDIRS = webextension
OBJ     = $(patsubst %.c, %.o, $(wildcard *.c))
JSFILES = $(wildcard scripts/*.js)

all: vimb $(SUBDIRS:%=%.subdir-all)

clean: $(SUBDIRS:%=%.subdir-clean)
	$(RM) vimb *.o scripts/scripts.h

vimb: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

$(OBJ): config.h $(BASEDIR)/config.mk scripts/scripts.h

-include $(OBJ:.o=.d)

config.h:
	@echo create $@ from config.def.h
	@cp config.def.h $@

scripts/scripts.h: $(JSFILES)
	$(RM) $@
	@echo "create $@ from *.js"
	@for file in $(JSFILES); do \
		./scripts/js2h.sh $$file >> $@; \
	done

%.o: %.c
	@echo "${CC} $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.subdir-all: config.h
	@$(MAKE) $(MFLAGS) -C $*

%.subdir-clean:
	@$(MAKE) $(MFLAGS) -C $* clean

.PHONY: all clean
