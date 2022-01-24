ROOT = $(realpath .)
SRC_DIR = $(ROOT)/src
SRC_DIRS = $(wildcard $(SRC_DIR)/*)
COMMON_DIR = $(SRC_DIR)/common
GAME_DIRS = $(filter-out $(COMMON_DIR),$(SRC_DIRS))
IDP_DEV_DIR = $(ROOT)/idp-dev
BIN_DIR = $(ROOT)/bin
IMG = $(BIN_DIR)/idp-games.img
HFE = $(BIN_DIR)/idp-games.hfe

SLIM = y

.PHONY: all
all: hfe

.PHONY: obj
obj: subdir-obj

.PHONY: hex
hex: subdir-hex

.PHONY: com
com: subdir-com
	mkdir -p $(BIN_DIR)
	@for dir in $(GAME_DIRS) ; do \
		echo cp $$dir/bin/\* $(BIN_DIR) ; \
		cp $$dir/bin/* $(BIN_DIR) ; \
	done

.PHONY: img
img: subdir-com
	mkdir -p $(BIN_DIR)
	cp $(IDP_DEV_DIR)/scripts/bootg.img $(IMG)
	@for dir in $(GAME_DIRS) ; do \
		for file in $$dir/bin/* ; do \
			echo cpmcp -f idpfdd $(IMG) $$file 0:$$(basename $$file) ; \
			(cd $(IDP_DEV_DIR)/scripts ; cpmcp -f idpfdd $(IMG) $$file 0:$$(basename $$file)) ; \
		done ; \
	done

.PHONY: hfe
hfe: img
	-hxcfe -uselayout:IDP -conv:HXC_HFE -finput:$(IMG) -foutput:$(HFE)

.PHONY: clean
clean:
	rm -rf $(BIN_DIR)
	@for dir in $(SRC_DIRS) ; do \
		echo $(MAKE) -C $$dir clean ; \
		$(MAKE) -C $$dir clean ; \
	done

.PHONY: idp-dev-clean
idp-dev-clean:
	$(MAKE) -C $(IDP_DEV_DIR) clean

.PHONY: clean-all
clean-all: clean idp-dev-clean

.PHONY: subdir-%
subdir-%:
	$(MAKE) -C $(IDP_DEV_DIR) all SLIM=$(SLIM)
	$(MAKE) -C $(COMMON_DIR) obj SLIM=$(SLIM)
	@for dir in $(GAME_DIRS) ; do \
		echo $(MAKE) -C $$dir $* SLIM=$(SLIM) ; \
		$(MAKE) -C $$dir $* SLIM=$(SLIM) ; \
	done