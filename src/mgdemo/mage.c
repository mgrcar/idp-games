#include <stdarg.h>
#include "../common/avdc.h"
#include "../common/gdp.h"
#include "../common/utils.h"
#include "mage.h"

#define _MAGE_BUFFER_WEIGHT_LIMIT 50
// setting to try if there are issues:
// * _MAGE_BUFFER_WEIGHT_LIMIT = 40
// * cursor move weight = 30
// * cursor OK weight = 10

typedef struct {
	uint16_t addr;
	uint8_t ch;
	uint8_t attr;
} _mage_buffer_item;

_mage_buffer_item _mage_render_buffer[_MAGE_BUFFER_WEIGHT_LIMIT];
_mage_buffer_item *_mage_buffer_ptr = _mage_render_buffer;
uint8_t _mage_buffer_weight = 0;

uint8_t _mage_dirty_row[5]; // 40 rows
uint8_t _mage_dirty_col_block[40][2]; // 13 column blocks in each row (16 bits available)
uint8_t _mage_dirty_char[40][13]; // 8 chars in each column block

uint8_t _mage_screen_buffer[40][100]; // 40 rows, 100 columns (200 x 120)

uint8_t _mage_init_str[10];
bool _mage_display_off;

void _mage_dirty_clear() {
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

void _mage_buffer_clear() {
	for (uint8_t row = 0; row < 40; row++) {
		for (uint8_t col = 0; col < 100; col++) {
			_mage_screen_buffer[row][col] = 0;
		}
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
	_mage_buffer_clear();
	_mage_dirty_clear();
	_mage_define_glyphs();
	uint8_t txt_attr;
	avdc_create_init_str(AVDC_MODE_132, 100, 40, 8, 6, &txt_attr, _mage_init_str);
	avdc_init_ex(AVDC_MODE_CUSTOM, txt_attr, _mage_init_str);
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

void _mage_flush() {
	avdc_wait_access();
	for (_mage_buffer_item *item = _mage_render_buffer; item < _mage_buffer_ptr; item++) {
		avdc_wait_ready();
		if (item->addr != 0) {
			AVDC_CUR_LWR = LO(item->addr);
			AVDC_CUR_UPR = HI(item->addr);
		}
		AVDC_CHR = item->ch;
		AVDC_ATTR = item->attr;
		AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
	}
	_mage_buffer_ptr = _mage_render_buffer;
	_mage_buffer_weight = 0;
}

void _mage_write_char(uint8_t row, uint8_t col, bool cursor_ok) {
	_mage_buffer_ptr->addr = cursor_ok ? 0 : avdc_get_pointer_cached(row, col);
	_mage_buffer_ptr->attr = AVDC_ATTR_UDG | ((_mage_screen_buffer[row][col] & 0xC0) >> 2);
	if ((_mage_screen_buffer[row][col] & 0xC0) == MAGE_ATTR_DIM) {
		_mage_buffer_ptr->ch = ~_mage_screen_buffer[row][col] & 0x3F;
	} else {
		_mage_buffer_ptr->ch = _mage_screen_buffer[row][col] & 0x3F;
	}
	_mage_buffer_ptr++;
}

void mage_render_faster() {
	avdc_set_cursor(0, 0);
	uint8_t r = 0;
	uint8_t c = 0;
	for (uint8_t row_byte = 0; row_byte < 5; row_byte++) {
		uint8_t dirty_row_byte = _mage_dirty_row[row_byte];
		if (dirty_row_byte != 0) {
			for (uint8_t row_bit = 0; row_bit < 8; row_bit++) {
				if ((dirty_row_byte & (1 << row_bit)) != 0) {
					uint8_t dirty_row = row_byte * 8 + row_bit;
					//printf("row: %u \n\r", dirty_row);
					for (uint8_t col_byte = 0; col_byte < 13; col_byte++) {
						if (_mage_dirty_char[dirty_row][col_byte] != 0) {
							uint8_t dirty_char_byte = _mage_dirty_char[dirty_row][col_byte];
							for (uint8_t char_bit = 0; char_bit < 8; char_bit++) {
								if ((dirty_char_byte & (1 << char_bit)) != 0) {
									//printf("%u %u \n\r", dirty_row, col_byte * 8 + char_bit);
									uint8_t col = col_byte * 8 + char_bit;
									bool cursor_ok = dirty_row == r && col == c;
									r = dirty_row;
									c = col + 1;
									uint8_t w = cursor_ok ? 10 : 25;
									if (_mage_buffer_weight + w > _MAGE_BUFFER_WEIGHT_LIMIT) {
										_mage_flush();
									}
									_mage_write_char(dirty_row, col, cursor_ok);
									_mage_buffer_weight += w;
								}
							}
							_mage_dirty_char[dirty_row][col_byte] = 0; // clear dirty char bits
						}
					}
				}
			}
			_mage_dirty_row[row_byte] = 0; // clear dirty row bits
		}
	}
	_mage_flush();
}


void mage_render_naive() {
	avdc_set_cursor(0, 0);
	uint8_t r = 0;
	uint8_t c = 0;
	for (uint8_t row = 0; row < 40; row++) {
		for (uint8_t col = 0; col < 100; col++) {
			if (_mage_screen_buffer[row][col] != 0) {
				bool cursor_ok = row == r && col == c;
				r = row;
				c = col + 1;
				uint8_t w = cursor_ok ? 10 : 25;
				if (_mage_buffer_weight + w > _MAGE_BUFFER_WEIGHT_LIMIT) {
					_mage_flush();
				}
				_mage_write_char(row, col, cursor_ok);
				_mage_buffer_weight += w;
			}
		}
	}
	_mage_flush();
}

void mage_render() {
	avdc_set_cursor(0, 0);
	uint8_t r = 0;
	uint8_t c = 0;
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
											uint8_t col = dirty_block * 8 + char_bit;
											bool cursor_ok = dirty_row == r && col == c;
											r = dirty_row;
											c = col + 1;
											uint8_t w = cursor_ok ? 10 : 25;
											if (_mage_buffer_weight + w > _MAGE_BUFFER_WEIGHT_LIMIT) {
												_mage_flush();
											}
											_mage_write_char(dirty_row, col, cursor_ok);
											_mage_buffer_weight += w;
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
	_mage_flush();
}

void mage_flag_dirty(uint8_t row, uint8_t col) {
	_mage_dirty_row[row >> 3] |= 1 << (row & 7);
	uint8_t block = col >> 3;
	_mage_dirty_col_block[row][block >> 3] |= 1 << (block & 7);
	_mage_dirty_char[row][block] |= 1 << (col & 7);
}

void mage_set_block(uint8_t row, uint8_t col, uint8_t val) {
	if (_mage_screen_buffer[row][col] != val) {
		_mage_screen_buffer[row][col] = val;
		mage_flag_dirty(row, col);
	}
}