BASEDIR=../..
include $(BASEDIR)/config.mk

OBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(EXTTARGET)

clean:
	$(RM) $(EXTTARGET) *.lo

$(EXTTARGET): $(OBJ)
	@echo "$(CC) $@"
	@$(CC) $(EXTLDFLAGS) ${OBJ} -o $@

%.lo: %.c
	@echo "${CC} $@"
	@$(CC) $(EXTCFLAGS) -fPIC -c -o $@ $<

.PHONY: all clean
