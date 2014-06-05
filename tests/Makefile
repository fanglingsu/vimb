include ../config.mk

CPPFLAGS += -I ../
CFLAGS   += -fPIC

TEST_PROGS = test-util

all: $(TEST_PROGS)
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." gtester --verbose $(TEST_PROGS)

${TEST_PROGS}: $(LIBTARGET)

clean:
	rm -f $(TEST_PROGS)
