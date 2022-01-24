ROOT = $(realpath .)
IDP_DEV_DIR = $(ROOT)/../../idp-dev
STD_LIB_INC = $(IDP_DEV_DIR)/lib/libcpm3-z80/include
INC_DIR = $(IDP_DEV_DIR)/include
BUILD_DIR = $(ROOT)/build
SLIM = 

CC = sdcc
CFLAGS = --std-c11 -mz80 -I$(STD_LIB_INC) -I. -I$(INC_DIR) --no-std-crt0 --nostdinc --nostdlib --debug -D SLIM=$(SLIM)

SRC = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.rel,$(SRC))

.PHONY: all
all: $(BUILD_DIR) $(OBJS)

.PHONY: $(BUILD_DIR)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -r -f $(BUILD_DIR)

$(OBJS):
	$(CC) -c -o $@ $< $(CFLAGS)

# Dependencies
$(BUILD_DIR)/invaders.rel: invaders.c invaders.h ../common/avdc.h ../common/utils.h ../common/gdp.h