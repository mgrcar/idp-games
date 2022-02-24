//#include <stdio.h>
#include <stdarg.h>
#include "../common/avdc.h"
#include "../common/gdp.h"
#include "mage.h"

uint8_t _mage_dirty_row[5]; // 40 rows
uint8_t _mage_dirty_col_block[40][2]; // 13 column blocks in each row (16 bits available)
uint8_t _mage_dirty_char[40][13]; // 8 chars in each column block

uint8_t _mage_screen[40][100]; // 40 rows, 100 columns (200 x 120)
uint8_t _mage_screen_attr[40][50]; // 4 bits per char

uint8_t _mage_init_str[10];

void _dirty_clear() {
	for (uint8_t i = 0; i < 5; i++) { 
		_mage_dirty_row[i] = 0; 
	}
	for (uint8_t i = 0; i < 40; i++) {
		for (uint8_t j = 0; j < 2; j++) {
			_mage_dirty_col_block[i][j] = 0;
		}
	}
	for (uint8_t i = 0; i < 40; i++) {
		for (uint8_t j = 0; j < 13; j++) {
			_mage_dirty_char[i][j] = 0;
		}
	}
}

void _mage_screen_clear() {
	for (uint8_t row = 0; row < 40; row++) {
		for (uint8_t col = 0; col < 100; col++) {
			_mage_screen[row][col] = 0;
		}
		for (uint8_t i = 0; i < 50; i++) {
			_mage_screen_attr[row][i] = 0;
		}
	}
}

uint8_t _bit(uint8_t val, uint8_t pos) {
	return (val & (1 << pos)) >> pos;
}

void _define_glyphs() {
	for (uint8_t char_code = 0; char_code < 64; char_code++) {
		uint8_t char_data[16];
		char_data[0] = char_data[1] = (_bit(char_code, 4) << 4) | _bit(char_code, 5);
		char_data[2] = char_data[3] = (_bit(char_code, 2) << 4) | _bit(char_code, 3);
		char_data[4] = char_data[5] = (_bit(char_code, 0) << 4) | _bit(char_code, 1);
		for (uint8_t j = 0; j < 6; j++) {
			char_data[j] |= char_data[j] << 2;
		}
		for (uint8_t j = 6; j < 16; j++) {
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

void mage_init() {
	gdp_cls();
	_mage_screen_clear();
	_dirty_clear();
	_define_glyphs();
	uint8_t txt_attr;
	avdc_create_init_str(AVDC_MODE_132, 100, 40, 8, 6, &txt_attr, _mage_init_str);
	avdc_init_ex(AVDC_MODE_CUSTOM, txt_attr, _mage_init_str);
}

void mage_done() {
	avdc_done();
}

void _mage_render_char(uint8_t row, uint8_t col) {
	avdc_set_cursor(row, col); // TODO: don't do it if already there (?)
	if ((_mage_screen[row][col] & 0xC0) == MAGE_ATTR_DIM) {
		avdc_write_at_cursor(~_mage_screen[row][col] & 0x3F, AVDC_ATTR_UDG | ((_mage_screen[row][col] & 0xC0) >> 2));
	} else {
		avdc_write_at_cursor(_mage_screen[row][col] & 0x3F, AVDC_ATTR_UDG | ((_mage_screen[row][col] & 0xC0) >> 2));
	}
}

void mage_render() {
	for (uint8_t row_byte = 0; row_byte < 5; row_byte++) {
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

void mage_set(uint8_t row, uint8_t col, uint8_t val) {
	if (_mage_screen[row][col] != val) {
		_mage_screen[row][col] = val;
		mage_flag_dirty(row, col);
	}
}

void mage_test_va(int num, ...)
{
va_list valist;
va_start(valist, num);
}