BASEDIR=..
SRCDIR=$(BASEDIR)/src
include $(BASEDIR)/config.mk

CPPFLAGS += -I $(BASEDIR)/
CFLAGS   += -fPIC -pedantic

TEST_PROGS = test-handlers \
			 test-map      \
			 test-shortcut \
			 test-util

all: $(TEST_PROGS)
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." gtester --verbose $(TEST_PROGS)

${TEST_PROGS}: $(SRCDIR)/$(LIBTARGET)

clean:
	$(RM) -f $(TEST_PROGS)
