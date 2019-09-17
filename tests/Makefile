CPPFLAGS = -I ../

include ../config.mk

TEST_PROGS = test-util \
			 test-shortcut \
			 test-handler \
			 test-file-storage

all: $(TEST_PROGS)
	$(Q)LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." gtester --verbose $(TEST_PROGS)

${TEST_PROGS}: ../$(SRCDIR)/vimb.so

test-%: test-%.c
	@echo "${CC} $@"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< ../$(SRCDIR)/vimb.so $(LDFLAGS)

clean:
	$(RM) $(TEST_PROGS)
