ROOT = $(realpath .)
IDP_DEV_ROOT = $(realpath ../..)/idp-dev
INC_DIR_STD_LIB = $(IDP_DEV_ROOT)/lib/libcpm3-z80/include
INC_DIR = $(IDP_DEV_ROOT)/include
BUILD_DIR = $(ROOT)/build
BIN_DIR = $(ROOT)/bin
BUILD_DIR_SDK = $(IDP_DEV_ROOT)/build
BUILD_DIR_COMMON = $(realpath ..)/common/build
AUX_DIR = $(realpath ../..)/make-aux

CC = sdcc
CFLAGS = --std-c11 -mz80 -I$(INC_DIR_STD_LIB) -I. -I$(INC_DIR) --no-std-crt0 --nostdinc --nostdlib --debug

LD = sdcc
LDFLAGS = -mz80 -Wl -y --code-loc 0x100 --no-std-crt0 --nostdlib --nostdinc -L$(BUILD_DIR_SDK) -llibsdcc-z80 -llibcpm3-z80 -llibpartner -p

L2 = sdldz80
L2FLAGS = -nf
L2FIX = sed '/-b _DATA = 0x8000/d'

TARGET = tetris

SRC = $(wildcard *.c)
BIN_SRC = $(wildcard *.bin)
BIN_DST = $(addprefix $(BIN_DIR)/,$(BIN_SRC))
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.rel,$(SRC))
OBJ_COMMON = $(wildcard $(BUILD_DIR_COMMON)/*.rel)
HEX = $(BUILD_DIR)/$(TARGET).ihx
COM = $(BIN_DIR)/$(TARGET).com
IMG = $(BIN_DIR)/$(TARGET).img
HFE = $(BIN_DIR)/$(TARGET).hfe

CRT0 = crt0cpm3-z80

.PHONY: all
all: com hfe

.PHONY: obj
obj: $(OBJ)

.PHONY: hex
hex: $(HEX)

.PHONY: bin com
bin com: $(COM) $(BIN_DST)
	@# Remove obsolete BIN files
	@for data_file in $(wildcard $(BIN_DIR)/*.bin) ; do \
		if [ ! -f $$(basename $$data_file) ] ; then \
			echo rm -f $$data_file ; \
			rm -f $$data_file ; \
		fi \
	done

.PHONY: img
img: $(IMG)

.PHONY: hfe
hfe: $(HFE)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# $(BIN_DST) ; Required for 'bin'/'com'
$(BIN_DIR)/%.bin: %.bin | $(BIN_DIR)
	cp $< $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# $(OBJ) ; Additional header dependencies below
$(BUILD_DIR)/%.rel: %.c %.h | $(BUILD_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(HEX): $(OBJ) $(OBJ_COMMON)
	$(LD) $(LDFLAGS) -o $(HEX) $(BUILD_DIR_SDK)/$(CRT0).rel $(OBJ) $(OBJ_COMMON)
	$(L2FIX) $(BUILD_DIR)/$(TARGET).lk > $(BUILD_DIR)/$(TARGET).link
	$(L2) $(L2FLAGS) $(BUILD_DIR)/$(TARGET).link

$(COM): $(HEX) | $(BIN_DIR)
	sdobjcopy -I ihex -O binary $(HEX) $(COM)

$(IMG): $(COM) $(BIN_SRC)
	cp $(AUX_DIR)/bootg.img $(IMG)
	@echo cpmcp -f idpfdd $(IMG) $(COM) 0:$(TARGET).com
	@(cd $(AUX_DIR) ; cpmcp -f idpfdd $(IMG) $(COM) 0:$(TARGET).com)
	@for data_file in $(BIN_SRC) ; do \
		echo cpmcp -f idpfdd $(IMG) $(ROOT)/$$data_file 0:$$data_file ; \
		(cd $(AUX_DIR) ; cpmcp -f idpfdd $(IMG) $(ROOT)/$$data_file 0:$$data_file) ; \
	done

$(HFE): $(IMG)
	-hxcfe -uselayout:IDP -conv:HXC_HFE -finput:$(IMG) -foutput:$(HFE)

# Additional header dependencies

$(BUILD_DIR)/$(TARGET).rel: ../common/avdc.h ../common/utils.h ../common/gdp.h