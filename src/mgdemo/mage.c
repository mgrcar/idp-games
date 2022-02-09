#include <stdio.h>
#include "../common/avdc.h"
#include "../common/gdp.h"
#include "mage.h"

uint8_t _mage_dirty_row[4]; // 31 rows (32 bits available)
uint8_t _mage_dirty_col_block[31][2]; // 16 column blocks in each row
uint8_t _mage_dirty_char[31][16]; // 8 chars in each column block

uint8_t _mage_screen[31][128]; // 31 rows, 128 columns (256 x 124)
uint8_t _mage_screen_attr[31][64]; // 4 bits per char

uint8_t _mage_init_str[10];

void _dirty_clear() {
	for (uint8_t i = 0; i < 4; i++) { 
		_mage_dirty_row[i] = 0; 
	}
	for (uint8_t i = 0; i < 31; i++) {
		for (uint8_t j = 0; j < 2; j++) {
			_mage_dirty_col_block[i][j] = 0;
		}
	}
	for (uint8_t i = 0; i < 31; i++) {
		for (uint8_t j = 0; j < 16; j++) {
			_mage_dirty_char[i][j] = 0;
		}
	}
}

void _mage_screen_clear() {
	for (uint8_t row = 0; row < 31; row++) {
		for (uint8_t col = 0; col < 128; col++) {
			_mage_screen[row][col] = 0;
		}
		for (uint8_t i = 0; i < 64; i++) {
			_mage_screen_attr[row][i] = 0;
		}
	}
}

uint8_t _bit(uint8_t val, uint8_t pos) {
	return (val & (1 << pos)) >> pos;
}

void _define_glyphs() {
	for (uint8_t char_code = 0; char_code < 128; char_code++) {
		uint8_t char_data[16];
		char_data[0] = char_data[1] = (_bit(char_code, 6) << 4);
		char_data[2] = char_data[3] = (_bit(char_code, 4) << 4) | _bit(char_code, 5);
		char_data[4] = char_data[5] = (_bit(char_code, 2) << 4) | _bit(char_code, 3);
		char_data[6] = char_data[7] = (_bit(char_code, 0) << 4) | _bit(char_code, 1);
		for (uint8_t j = 0; j < 8; j++) {
			char_data[j] |= char_data[j] << 2;
		}
		for (uint8_t j = 8; j < 16; j++) {
			char_data[j] = 0;
		}
		avdc_define_glyph(char_code, char_data);
	}
}

void _display_off() {
	// firing the AVDC "display off" and "display on" commands causes glitching, so we do a reset instead
	// this code is taken from avdc.c (see avdc_reset)
	avdc_wait_access();
    avdc_wait_ready();
    AVDC_CMD = AVDC_CMD_RESET;
    while ((AVDC_ACCESS & AVDC_ACCESS_FLAG) != 0);
    AVDC_CMD = AVDC_CMD_RESET;
    AVDC_COMMON_TXT_ATTR = 0xC4; // WARNME is it?
    while ((AVDC_GDP_STATUS & 2) != 2);
    while ((AVDC_GDP_STATUS & 2) == 2);
    AVDC_SCREEN_START_2_LOWER = 0;
    AVDC_SCREEN_START_2_UPPER = 0;
    AVDC_CMD = AVDC_CMD_SET_REG_0;
    for (uint8_t i = 0; i < 10; i++) {
        AVDC_INIT = _mage_init_str[i];
    }
    AVDC_SCREEN_START_2_LOWER = 0;
    AVDC_SCREEN_START_2_UPPER = 0;
}

void _display_on() {
	AVDC_CMD = AVDC_CMD_DISPLAY_ON;
}

extern uint8_t _init_str_132[];

void mage_init() {
	gdp_cls();
	_mage_screen_clear();
	_dirty_clear();
	//_define_glyphs();
	uint8_t txt_attr;
	avdc_create_init_str(AVDC_MODE_132, 132, 26, 8, 11, &txt_attr, _mage_init_str);
	//avdc_init_ex(AVDC_MODE_CUSTOM, txt_attr, _mage_init_str);
	avdc_init();
	//avdc_init_ex(AVDC_MODE_132, 0, 0);
	//avdc_init_ex(AVDC_MODE_CUSTOM, 0xC4, _init_str_132);
}

void mage_done() {
	//avdc_done();
}

void _mage_render_char(uint8_t row, uint8_t col) {
	avdc_set_cursor(row, col); // TODO: don't do it if already there (?)
	uint8_t block = _mage_screen[row][col];
	uint8_t attr = _mage_screen_attr[row][col >> 1];
	if ((col & 1) == 0) { attr <<= 4; } else { attr &= 0xF0; }
	if (block & 128) {
		avdc_write_at_cursor(~block & 127, 0x10 | 0x30);
		//printf("attr: %u \n\r", AVDC_ATTR_UDG | AVDC_ATTR_INVERT | attr);
	} else {
		avdc_write_at_cursor(block & 127, 0x10 | 0x30);
		//printf("attr: %u \n\r", AVDC_ATTR_UDG | attr);
	}
}

void mage_render() {
	for (uint8_t row_byte = 0; row_byte < 4; row_byte++) {
		uint8_t dirty_row_byte = _mage_dirty_row[row_byte];
		if (dirty_row_byte != 0) {
			for (uint8_t row_bit = 0; row_bit < 8; row_bit++) {
				if ((dirty_row_byte & (1 << row_bit)) != 0) {
					uint8_t dirty_row = row_byte * 8 + row_bit;
					//printf("row: %u \n\r", dirty_row);
					for (uint8_t block_byte = 0; block_byte < 2; block_byte++) {
						uint8_t dirty_block_byte = _mage_dirty_col_block[dirty_row][block_byte];
						if (dirty_block_byte != 0) {
							for (uint8_t block_bit = 0; block_bit < 8; block_bit++) {
								if ((dirty_block_byte & (1 << block_bit)) != 0) {
									uint8_t dirty_block = block_byte * 8 + block_bit;
									//printf("block: %u \n\r", dirty_block);
									uint8_t dirty_char_byte = _mage_dirty_char[dirty_row][dirty_block];
									for (uint8_t char_bit = 0; char_bit < 8; char_bit++) {
										if ((dirty_char_byte & (1 << char_bit)) != 0) {
											//printf("%u %u \n\r", dirty_row, dirty_block * 8 + char_bit);
											_mage_render_char(dirty_row, dirty_block * 8 + char_bit);
										}
									}
									_mage_dirty_char[dirty_row][dirty_block] = 0; // clear dirty char bits
								}
							}
							_mage_dirty_col_block[dirty_row][block_byte] = 0; // clear dirty block bits
						}
					}
				}
			}
			_mage_dirty_row[row_byte] = 0; // clear dirty row bits
		}
	}
}

void mage_flag_dirty(uint8_t row, uint8_t col) {
	_mage_dirty_row[row >> 3] |= 1 << (row & 7);
	uint8_t block = col >> 3;
	_mage_dirty_col_block[row][block >> 3] |= 1 << (block & 7);
	_mage_dirty_char[row][block] |= 1 << (col & 7);
}

void mage_set_block(uint8_t row, uint8_t col, uint8_t bits) {
	if (_mage_screen[row][col] != bits) {
		_mage_screen[row][col] = bits;
		mage_flag_dirty(row, col);
	}
}

void mage_set_block_attr(uint8_t row, uint8_t col, uint8_t attr) {
	uint8_t val = _mage_screen_attr[row][col >> 1];
	if ((col & 1) == 0) { // 0, 2, 4...
		if ((val & 0x0F) != attr) {
			_mage_screen_attr[row][col >> 1] = (val & 0xF0) | attr;
			mage_flag_dirty(row, col);
		}
	} else { // 1, 3, 5...
		if ((val & 0xF0) != (attr << 4)) {
			_mage_screen_attr[row][col >> 1] = (val & 0x0F) | (attr << 4);
			mage_flag_dirty(row, col);
		}
	}
}