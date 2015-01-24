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
	@echo "$(CC) $@"
	@$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(LIBTARGET): $(LOBJ)
	@echo "$(CC) $@"
	@$(CC) -shared ${LOBJ} -o $@ $(LDFLAGS)

config.h:
	@echo create $@ from config.def.h
	@cp config.def.h $@

%.o: %.c %.h
	@echo "${CC} $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.lo: %.c %.h
	@echo "${CC} $@"
	@$(CC) -DTESTLIB $(CFLAGS) -fPIC -c -o $@ $<

-include $(OBJ:.o=.d)

.PHONY: all clean clean-lib
