include config.mk

all: $(TARGET)

-include $(DEPS)

options:
	@echo "$(PROJECT) build options:"
	@echo "LIBS      = $(LIBS)"
	@echo "CC        = $(CC)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "CPPFLAGS  = $(CPPFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "$(CC) $@"
	@$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(HEAD)
	@echo "${CC} $<"
	@$(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

debug: $(DTARGET)

$(DTARGET): $(DOBJ)
	@echo "$(CC) $@"
	@$(CC) $(DFLAGS) $(DOBJ) -o $(DTARGET) $(LDFLAGS)

%.do: %.c $(HEAD)
	@echo "${CC} $<"
	@$(CC) -c -o $@ $< $(CPPFLAGS) $(DFLAGS)

clean:
	$(RM) $(OBJ) $(DOBJ) $(TARGET) $(DTARGET)

dist: distclean
	@echo "Creating tarball."
	@git archive --format tar -o $(DIST_FILE) HEAD

distclean:
	$(RM) $(DIST_FILE)

.PHONY: clean debug all install uninstall options dist
