BASEDIR=../..
include $(BASEDIR)/config.mk

OBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(EXTTARGET)

clean:
	$(RM) $(EXTTARGET) *.lo

$(EXTTARGET): $(OBJ)
	@echo "$(CC) $@"
	$(Q)$(CC) $(OBJ) $(EXT_LDFLAGS) -o $@

%.lo: %.c
	@echo "${CC} $@"
	$(Q)$(CC) $(EXT_CFLAGS) -fPIC -c -o $@ $<

.PHONY: all clean
