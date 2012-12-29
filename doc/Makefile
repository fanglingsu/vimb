include ../config.mk

all: man

man: vimp.1

%.1: %.1.txt
	@echo "${PP} $< > $@"
	@$(PP) $(CPPFLAGS) $< > $@

clean:
	$(RM) *.1

.PHONY: clean
