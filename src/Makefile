SUBDIRS = common tetris tetrisg invaders
SUBDIRSCLEAN = $(addsuffix _clean,$(SUBDIRS))
SUBDIRSALL = $(addsuffix _all,$(SUBDIRS))

.PHONY: all
all: $(SUBDIRSALL)

.PHONY: clean
clean: $(SUBDIRSCLEAN)

.PHONY: %_all
%_all: %
	$(MAKE) -C $<

PHONY: %_clean
%_clean: %
	$(MAKE) -C $< clean
