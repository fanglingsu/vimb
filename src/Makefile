BASEDIR=..
include $(BASEDIR)/config.mk

SUBDIRS = scripts webextension
OBJ     = $(patsubst %.c, %.o, $(wildcard *.c))

all: vimb

clean: $(SUBDIRS:%=%.subdir-clean)
	$(RM) vimb *.o

vimb: $(OBJ) webextension.subdir-all
	$(CC) $(LDFLAGS) $(OBJ) -o $@

$(OBJ): scripts.subdir-all config.h $(BASEDIR)/config.mk

-include $(OBJ:.o=.d)

config.h:
	@echo create $@ from config.def.h
	@cp config.def.h $@

%.o: %.c
	@echo "${CC} $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.subdir-all: config.h
	@$(MAKE) $(MFLAGS) -C $*

%.subdir-clean:
	@$(MAKE) $(MFLAGS) -C $* clean

.PHONY: all clean
