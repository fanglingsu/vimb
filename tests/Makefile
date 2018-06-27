CPPFLAGS = -I ../

include ../config.mk

TEST_PROGS = test-util \
			 test-shortcut

all: $(TEST_PROGS)
	$(Q)LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." gtester --verbose $(TEST_PROGS)

${TEST_PROGS}: ../$(SRCDIR)/vimb.so

test-%: test-%.c
	@echo "${CC} $@"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< ../$(SRCDIR)/vimb.so

clean:
	$(RM) $(TEST_PROGS)
