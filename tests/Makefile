include ../config.mk

CPPFLAGS += -I ../
CFLAGS   += -fPIC -Wpedantic

TEST_PROGS = test-handlers \
			 test-map      \
			 test-shortcut \
			 test-util

all: $(TEST_PROGS)
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." gtester --verbose $(TEST_PROGS)

${TEST_PROGS}: $(notdir $(LIBTARGET))

clean:
	rm -f $(TEST_PROGS)
