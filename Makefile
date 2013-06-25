include config.mk

-include $(DEPS)

all: $(TARGET) man

options:
	@echo "$(PROJECT) build options:"
	@echo "LIBS      = $(LIBS)"
	@echo "CC        = $(CC)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "CPPFLAGS  = $(CPPFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"

src/hints.o: src/hints.js.h
src/hints.js.h: src/hints.js
	@echo "minify $<"
	@cat $< | src/js2h.sh > $@

$(TARGET): $(OBJ)
	@echo "$(CC) $@"
	@$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c %.h src/config.h
	@echo "${CC} $<"
	@$(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

debug: $(DTARGET)

$(DTARGET): $(DOBJ)
	@echo "$(CC) $@"
	@$(CC) $(DFLAGS) $(DOBJ) -o $(DTARGET) $(LDFLAGS)

%.do: %.c $(HEAD)
	@echo "${CC} $<"
	@$(CC) -c -o $@ $< $(CPPFLAGS) $(DFLAGS)

man:
	@$(MAKE) $(MFLAGS) -C doc man

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)
	@$(MAKE) $(MFLAGS) -C doc install

uninstall:
	$(RM) $(BINDIR)$(TARGET)
	@$(MAKE) $(MFLAGS) -C doc uninstall

clean:
	@$(MAKE) $(MFLAGS) -C doc clean
	$(RM) src/*.o src/*.do src/hints.js.h $(TARGET) $(DTARGET)

dist: distclean
	@echo "Creating tarball."
	@git archive --format tar -o $(DIST_FILE) HEAD

distclean:
	$(RM) $(DIST_FILE)

.PHONY: clean debug all install uninstall options dist
