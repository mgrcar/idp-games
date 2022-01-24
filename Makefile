ROOT = $(realpath .)
BIN_DIR = $(ROOT)/bin

SUBDIRS = idp-dev src
SUBDIRSCLEAN = $(addsuffix _clean,$(SUBDIRS))
SUBDIRSALL = $(addsuffix _all,$(SUBDIRS))

.PHONY: all
all: $(BIN_DIR) $(SUBDIRSALL)

.PHONY: clean
clean: clean_local $(SUBDIRSCLEAN)

.PHONY: %_all
%_all: %
	$(MAKE) -C $<

.PHONY: %_clean
%_clean: %
	$(MAKE) -C $< clean

.PHONY: $(BIN_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

.PHONY: clean_local
clean_local: 
	rm -r -f $(BIN_DIR)