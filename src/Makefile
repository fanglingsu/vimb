include ../config.mk

OBJ = $(patsubst %.c, %.o, $(wildcard *.c))

all: vimb webextension.subdir-all

clean: webextension.subdir-clean
	$(RM) vimb $(OBJ)

vimb: $(OBJ)
	@echo "${CC} $@"
	$(Q)$(CC) $(OBJ) $(LDFLAGS) -o $@

vimb.so: $(OBJ)
	$(Q)$(CC) -shared $(OBJ) $(LDFLAGS) -o $@

$(OBJ): config.h ../config.mk

normal.o: scripts/scripts.h

setting.o: scripts/scripts.h

scripts/scripts.h: scripts.subdir-all

config.h:
	@echo create $@ from config.def.h
	$(Q)cp config.def.h $@

%.o: %.c
	@echo "${CC} $@"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.subdir-all: config.h
	$(Q)$(MAKE) -C $*

%.subdir-clean:
	$(Q)$(MAKE) -C $* clean

-include $(OBJ:.o=.d)

.PHONY: all clean
