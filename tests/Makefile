CPPFLAGS = -I ../
CFLAGS   = -fPIC -pedantic

include ../config.mk

TEST_PROGS = test-util

all: $(TEST_PROGS)
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." gtester --verbose $(TEST_PROGS)

${TEST_PROGS}: ../$(SRCDIR)/vimb.so

clean:
	$(RM) -f $(TEST_PROGS)
