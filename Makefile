ROOT = $(realpath .)
SRC_DIR = $(ROOT)/src
SRC_DIRS = $(wildcard $(SRC_DIR)/*)
COMMON_DIR = $(SRC_DIR)/common
GAME_DIRS = $(filter-out $(COMMON_DIR),$(SRC_DIRS))
IDP_DEV_DIR = $(ROOT)/idp-dev
BIN_DIR = $(ROOT)/bin
IMG = $(BIN_DIR)/idp-games.img
HFE = $(BIN_DIR)/idp-games.hfe
AUX_DIR = $(ROOT)/make-aux

SLIM = y

.PHONY: all
all: hfe

.PHONY: sdk
sdk:
	$(MAKE) -C $(IDP_DEV_DIR) libs SLIM=$(SLIM)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

.PHONY: bin com
bin com: subdir-bin | $(BIN_DIR)
	@for dir in $(GAME_DIRS) ; do \
		find $$dir/bin \( -name *.bin -or -name *.com \) -exec sh -c '\
			echo cp {} $(BIN_DIR) ; \
			cp {} $(BIN_DIR) \
			' ';' ; \
	done

.PHONY: img
img: subdir-bin | $(BIN_DIR)
	cp $(AUX_DIR)/bootg.img $(IMG)
	@for dir in $(GAME_DIRS) ; do \
		find $$dir/bin \( -name *.bin -or -name *.com \) -exec sh -c '\
			echo cpmcp -f idpfdd $(IMG) {} 0:$$(basename {}) ; \
			(cd $(AUX_DIR) ; cpmcp -f idpfdd $(IMG) {} 0:$$(basename {})) \
			' ';' ; \
	done

.PHONY: hfe
hfe: img
	-hxcfe -uselayout:IDP -conv:HXC_HFE -finput:$(IMG) -foutput:$(HFE)

.PHONY: clean
clean: subdir-clean
	rm -rf $(BIN_DIR)

.PHONY: sdk-clean clean-sdk
sdk-clean clean-sdk:
	$(MAKE) -C $(IDP_DEV_DIR) clean

.PHONY: subdir-clean
subdir-clean:
	@for dir in $(SRC_DIRS) ; do \
		echo $(MAKE) -C $$dir clean ; \
		$(MAKE) -C $$dir clean ; \
	done

.PHONY: subdir-%
subdir-%:
	$(MAKE) -C $(COMMON_DIR) obj || exit
	@for dir in $(GAME_DIRS) ; do \
		echo $(MAKE) -C $$dir $* '||' exit ; \
		$(MAKE) -C $$dir $* || exit ; \
	done