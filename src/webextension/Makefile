BASEDIR=../..
include $(BASEDIR)/config.mk

OBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(EXTTARGET)

clean:
	$(RM) $(EXTTARGET) *.lo

$(EXTTARGET): $(OBJ)
	@echo "$(CC) $@"
	$(Q)$(CC) $(OBJ) $(EXTLDFLAGS) -o $@

%.lo: %.c
	@echo "${CC} $@"
	$(Q)$(CC) $(EXTCFLAGS) -fPIC -c -o $@ $<

.PHONY: all clean
