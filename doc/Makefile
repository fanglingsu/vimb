include ../config.mk

all: man

man: vimp.1

%.1: %.1.txt
	@echo "${PP} $< > $@"
	@$(PP) -DVERSION=$(VERSION) -DPROJECT=$(PROJECT) $< > $@

clean:
	$(RM) *.1

.PHONY: clean
