BASEDIR=../..
include $(BASEDIR)/config.mk

OBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(EXTTARGET)

clean:
	$(RM) $(EXTTARGET) *.lo

$(EXTTARGET): $(OBJ)
	$(Q)echo "$(CC) $@"
	$(Q)$(CC) $(OBJ) $(EXTLDFLAGS) -o $@

%.lo: %.c
	$(Q)echo "${CC} $@"
	$(Q)$(CC) $(EXTCFLAGS) -fPIC -c -o $@ $<

.PHONY: all clean
