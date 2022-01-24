ROOT = $(realpath .)
IDP_DEV_ROOT = $(realpath ../..)/idp-dev
INC_DIR_STD_LIB = $(IDP_DEV_ROOT)/lib/libcpm3-z80/include
INC_DIR = $(IDP_DEV_ROOT)/include
BUILD_DIR = $(ROOT)/build
SLIM = 

CC = sdcc
CFLAGS = --std-c11 -mz80 -I$(INC_DIR_STD_LIB) -I. -I$(INC_DIR) --no-std-crt0 --nostdinc --nostdlib --debug -D SLIM=$(SLIM)

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.rel,$(SRC))

.PHONY: all
all: obj

.PHONY: obj
obj: $(OBJ)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# $(OBJ) ; Additional header dependencies below
$(BUILD_DIR)/%.rel: %.c %.h
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

# Additional header dependencies

$(BUILD_DIR)/utils.rel: avdc.h
$(BUILD_DIR)/gdp.rel: utils.h
$(BUILD_DIR)/avdc.rel: utils.h