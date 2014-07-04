include config.mk

all:   $(TARGET)
debug: $(DTARGET)
test:  $(LIBTARGET)
	@$(MAKE) $(MFLAGS) -s -C tests

options:
	@echo "$(PROJECT) build options:"
	@echo "LIBS    = $(LIBS)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

install: $(TARGET) doc/$(MAN1)
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(MANDIR1)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "install -m 644 src/$(MAN1) $(DESTDIR)$(MANDIR1)/$(MAN1)"
	@sed -e "s/VERSION/$(VERSION)/g" \
		-e "s/DATE/`date +'%m %Y'`/g" < doc/$(MAN1) > $(DESTDIR)$(MANDIR1)/$(MAN1)
	@chmod 644 $(DESTDIR)$(MANDIR1)/$(MAN1)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)
	$(RM) $(DESTDIR)$(MANDIR1)/$(MAN1)

clean: test-clean
	$(RM) src/*.o src/*.do src/*.lo src/hints.js.h
	$(RM) $(TARGET) $(DTARGET)

test-clean:
	$(RM) $(LIBTARGET)
	@$(MAKE) $(MFLAGS) -C tests clean

dist: dist-clean
	@echo "Creating tarball."
	@git archive --format tar -o $(DIST_FILE) HEAD

dist-clean:
	$(RM) $(DIST_FILE)

src/hints.o:  src/hints.js.h
src/hints.do: src/hints.js.h
src/hints.lo: src/hints.js.h

src/hints.js.h: src/hints.js
	@echo "minify $<"
	@cat $< | src/js2h.sh > $@

$(OBJ):  src/config.h config.mk
$(DOBJ): src/config.h config.mk
$(LOBJ): src/config.h config.mk

$(TARGET): $(OBJ)
	@echo "$(CC) $@"
	@$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(DTARGET): $(DOBJ)
	@echo "$(CC) $@"
	@$(CC) $(DOBJ) -o $@ $(DLDFLAGS)

$(LIBTARGET): $(LOBJ)
	@echo "$(CC) $@"
	@$(CC) -shared ${LOBJ} -o $@ $(LDFLAGS)

src/config.h:
	@echo create $@ from src/config.def.h
	@cp src/config.def.h $@

%.o: %.c %.h
	@echo "${CC} $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.do: %.c %.h
	@echo "${CC} $@"
	@$(CC) $(DFLAGS) -c -o $@ $<

%.lo: %.c %.h
	@echo "${CC} $@"
	@$(CC) -DTESTLIB $(DFLAGS) -fPIC -c -o $@ $<

-include $(DEPS)

.PHONY: clean debug all install uninstall options dist dist-clean test test-clean
