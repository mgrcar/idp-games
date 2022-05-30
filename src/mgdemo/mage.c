#include "../common/avdc.h"
#include "../common/gdp.h"
#include "../common/utils.h"
#include "mage.h"

uint8_t _mage_screen_buffer[40][100][2]; // 40 rows, 100 columns (200 x 120), 2 values (char, attr)

bool _mage_display_off;

void _mage_screen_buffer_clear() {
	uint8_t *ptr = (uint8_t *)_mage_screen_buffer;
	for (uint16_t i = 0; i < 40 * 100 * 2; i++, ptr++) {
		*ptr = 0;
	}
}

uint8_t _mage_get_bit(uint8_t val, uint8_t pos) {
	return (val & (1 << pos)) >> pos;
}

void _mage_define_glyphs() {
	for (uint8_t char_code = 0; char_code < 64; char_code++) {
		uint8_t char_data[16];
		char_data[0] = char_data[1] = (_mage_get_bit(char_code, 4) << 4) | _mage_get_bit(char_code, 5);
		char_data[2] = char_data[3] = (_mage_get_bit(char_code, 2) << 4) | _mage_get_bit(char_code, 3);
		char_data[4] = char_data[5] = (_mage_get_bit(char_code, 0) << 4) | _mage_get_bit(char_code, 1);
		for (uint8_t j = 0; j < 6; j++) {
			char_data[j] |= char_data[j] << 2;
		}
		for (uint8_t j = 6; j < 16; j++) {
			char_data[j] = 0;
		}
		avdc_define_glyph(char_code, char_data);
	}
}

void mage_init() {
	gdp_cls();
	_mage_screen_buffer_clear();
	_mage_define_glyphs();
	uint8_t txt_attr;
	uint8_t init_str[10];
	avdc_create_init_str(AVDC_MODE_132, 100, 40, 8, 6, &txt_attr, init_str);
	avdc_init_ex(AVDC_MODE_CUSTOM, txt_attr, init_str);
	_mage_display_off = false;
}

void mage_done() {
	avdc_done();
}

void mage_display_off() {
	_mage_display_off = true;
	avdc_display_off();
}

void mage_display_on() {
	_mage_display_off = false;
	avdc_display_on();
}

void mage_set_block(uint8_t row, uint8_t col, uint8_t val, uint8_t attr) {
	uint16_t addr = row * 100 + col;
	uint8_t *ptr = (uint8_t *)_mage_screen_buffer + (addr << 1) + 1;
	if (*(ptr - 1) != val || (*ptr & 63) != attr) {
		*(ptr - 1) = (attr == MAGE_ATTR_DIM) ? (~val & 63) : val;
		*ptr = attr | 128;
		*(ptr - ((addr & 7) << 1)) |= 64; // dirty block flag
	}
}

void mage_set_block_raw(uint16_t addr, uint8_t val, uint8_t attr) {
	uint8_t *ptr = (uint8_t *)_mage_screen_buffer + (addr << 1);
	*ptr = val;
	*(ptr + 1) = attr;
}

void mage_render_blocks(uint8_t count, uint8_t *blocks) { // NOTE: you can render up to 4 blocks
	if (!_mage_display_off) { avdc_wait_access(); }
	uint8_t *ptr = blocks;
	for (uint8_t i = 0; i < count; i++) {
		avdc_wait_ready();
		AVDC_CHR = *ptr;
		ptr++;
		AVDC_ATTR = AVDC_ATTR_UDG | *ptr;
		ptr++;
		AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
	}
}

void mage_render_block_at(uint16_t addr, uint8_t ch, uint8_t attr) {
	addr += 80;
	if (!_mage_display_off) { avdc_wait_access(); }
	avdc_wait_ready();
	AVDC_CUR_LWR = LO(addr);
	AVDC_CUR_UPR = HI(addr);
	AVDC_CHR = ch;
	AVDC_ATTR = AVDC_ATTR_UDG | attr;
	AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
}

void mage_render_range(uint16_t addr, uint16_t addr_end) {
	uint8_t *ptr = (uint8_t *)_mage_screen_buffer + (addr << 1) + 1;
	addr += 80;
	addr_end += 80;
	goto step;
jump:
	while ((*ptr & 64) == 0) {
		ptr += 16;
		addr += 8;
		if (addr >= addr_end) { return; }
	}
step:
	while ((*ptr & 128) == 0) {
		if ((addr & 7) == 0 && (*ptr & 64) == 0) {
			goto jump;
		}
		*ptr &= 63;
		ptr += 2;
		addr++;
		if (addr == addr_end) { return; }
	}
	// here we move the cursor (this is very slow, so we need to call avdc_wait_access)
	*ptr &= 63;
	if (!_mage_display_off) { avdc_wait_access(); }
	avdc_wait_ready();
	AVDC_CUR_LWR = LO(addr);
	AVDC_CUR_UPR = HI(addr);
	AVDC_CHR = *(ptr - 1);
	AVDC_ATTR = AVDC_ATTR_UDG | *ptr;
	AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
	ptr += 2;
	addr++;
	if (addr == addr_end) { return; }
	uint8_t count = 0;
	while ((*ptr & 128) != 0) {
		// here we write chars, up to 4 within one avdc_wait_access
		*ptr &= 63;
		if ((count & 3) == 0 && !_mage_display_off) {
			avdc_wait_access();
			//count = 0; // otherwise count could go over 255 (what happens if it goes over?)
		}
		avdc_wait_ready();
		AVDC_CHR = *(ptr - 1);
		AVDC_ATTR = AVDC_ATTR_UDG | *ptr;
		AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
		ptr += 2;
		addr++;
		if (addr == addr_end) { return; }
		count++;
	}
	goto step;
}

void _mage_render_full() {
	uint8_t *ptr = (uint8_t *)_mage_screen_buffer + 1;
	uint16_t addr = 80;
jump:
	while ((*ptr & 64) == 0) {
		ptr += 16;
		addr += 8;
		if (addr == 4080) { return; }
	}
step:
	while ((*ptr & 128) == 0) {
		if ((addr & 7) == 0 && (*ptr & 64) == 0) {
			goto jump;
		}
		*ptr &= 63;
		ptr += 2;
		addr++;
		if (addr == 4080) { return; }
	}
	// here we move the cursor (this is very slow, so we need to call avdc_wait_access)
	*ptr &= 63;
	if (!_mage_display_off) { avdc_wait_access(); }
	avdc_wait_ready();
	AVDC_CUR_LWR = LO(addr);
	AVDC_CUR_UPR = HI(addr);
	AVDC_CHR = *(ptr - 1);
	AVDC_ATTR = AVDC_ATTR_UDG | *ptr;
	AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
	ptr += 2;
	addr++;
	if (addr == 4080) { return; }
	uint8_t count = 0;
	while ((*ptr & 128) != 0) {
		// here we write chars, up to 4 within one avdc_wait_access
		*ptr &= 63;
		if ((count & 3) == 0 && !_mage_display_off) {
			avdc_wait_access();
			//count = 0; // otherwise count could go over 255 (what happens if it goes over?)
		}
		avdc_wait_ready();
		AVDC_CHR = *(ptr - 1);
		AVDC_ATTR = AVDC_ATTR_UDG | *ptr;
		AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
		ptr += 2;
		addr++;
		if (addr == 4080) { return; }
		count++;
	}
	goto step;
}

void mage_render(bool interlaced) {
	if (interlaced && !_mage_display_off) {
		for (uint16_t addr = 0; addr < 4000; addr += 200) {
			mage_render_range(addr, addr + 100);
		}
		for (uint16_t addr = 100; addr < 4000; addr += 200) {
			mage_render_range(addr, addr + 100);
		}
	} else {
		_mage_render_full();
	}
}

void mage_set_row_addr(uint8_t row, uint16_t addr) {
	addr += 80;
	uint8_t write_addr = row << 1;
	// set cursor
	if (!_mage_display_off) { avdc_wait_access(); }
	avdc_wait_ready();
	AVDC_CUR_LWR = write_addr;
    AVDC_CUR_UPR = 0;
    // first address byte
	AVDC_CHR = LO(addr);
	AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
	// second address byte
	avdc_wait_ready();
	AVDC_CHR = HI(addr);
	AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
}