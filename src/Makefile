BASEDIR=..
include $(BASEDIR)/config.mk

OBJ  = $(patsubst %.c, %.o, $(wildcard *.c))
LOBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(TARGET)

clean: clean-lib
	$(RM) $(TARGET) *.o *.lo hints.js.h

clean-lib:
	$(RM) $(LIBTARGET)

hints.o:  hints.js.h
hints.lo: hints.js.h

hints.js.h: hints.js
	@echo "minify $<"
	@cat $< | ./js2h.sh > $@

$(OBJ):  config.h $(BASEDIR)/config.mk
$(LOBJ): config.h $(BASEDIR)/config.mk

$(TARGET): $(OBJ)
ifeq ($(VERBOSE),0)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
else
	@echo "$(CC) $@"
	@$(CC) $(OBJ) -o $@ $(LDFLAGS)
endif

$(LIBTARGET): $(LOBJ)
ifeq ($(VERBOSE),0)
	@echo "$(CC) $@"
	@$(CC) -shared ${LOBJ} -o $@ $(LDFLAGS)
else
	$(CC) -shared ${LOBJ} -o $@ $(LDFLAGS)
endif

config.h:
	@echo create $@ from config.def.h
	@cp config.def.h $@

%.o: %.c %.h
ifeq ($(VERBOSE),0)
	@echo "${CC} $@"
	@$(CC) $(CFLAGS) -c -o $@ $<
else
	$(CC) $(CFLAGS) -c -o $@ $<
endif

%.lo: %.c %.h
ifeq ($(VERBOSE),0)
	@echo "${CC} $@"
	@$(CC) -DTESTLIB $(CFLAGS) -fPIC -c -o $@ $<
else
	$(CC) -DTESTLIB $(CFLAGS) -fPIC -c -o $@ $<
endif

-include $(OBJ:.o=.d)

.PHONY: all clean clean-lib
