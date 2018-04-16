include ../../config.mk

OBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(EXTTARGET)

clean:
	$(RM) $(EXTTARGET) $(OBJ)

$(EXTTARGET): $(OBJ)
	@echo "$(CC) $@"
	$(Q)$(CC) $(OBJ) $(EXTLDFLAGS) -o $@

%.lo: %.c
	@echo "${CC} $@"
	$(Q)$(CC) $(EXTCPPFLAGS) $(EXTCFLAGS) -fPIC -c -o $@ $<

-include $(OBJ:.lo=.d)

.PHONY: all clean
