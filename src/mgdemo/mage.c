#include "../common/avdc.h"
#include "../common/gdp.h"
#include "../common/utils.h"
#include "mage.h"

#define _MAGE_BUFFER_WEIGHT_LIMIT 50
#define _MAGE_BUFFER_MOVE_AND_WRITE_WEIGHT 25
#define _MAGE_BUFFER_WRITE_WEIGHT 10
#define _MAGE_BUFFER_MAX_SIZE 5
// "safer" settings:
// _MAGE_BUFFER_WEIGHT_LIMIT 40
// _MAGE_BUFFER_MOVE_AND_WRITE_WEIGHT 30
// _MAGE_BUFFER_WRITE_WEIGHT 10
// _MAGE_BUFFER_MAX_SIZE 4

typedef struct {
	uint16_t addr;
	uint8_t ch;
	uint8_t attr;
} _mage_render_buffer_item;

_mage_render_buffer_item _mage_render_buffer[_MAGE_BUFFER_MAX_SIZE];
_mage_render_buffer_item *_mage_render_buffer_ptr = _mage_render_buffer;
uint8_t _mage_render_buffer_weight = 0;

uint8_t _mage_dirty_row[5]; // 40 rows
uint8_t _mage_dirty_char[40][13]; // 40 rows, max 104 columns

uint8_t _mage_screen_chr[40][100][2]; // 40 rows, 100 columns (200 x 120), 2 data bytes (char, attr)

uint16_t _mage_row_ptr[40];
uint8_t _mage_dirty_row_ptr[5];

uint8_t _mage_init_str[10];
bool _mage_display_off;

void _mage_dirty_clear() {
	for (uint8_t i = 0; i < 5; i++) { 
		_mage_dirty_row[i] = 0; 
	}
	for (uint8_t i = 0; i < 40; i++) {
		for (uint8_t j = 0; j < 13; j++) {
			_mage_dirty_char[i][j] = 0;
		}
	}
	for (uint8_t i = 0; i < 5; i++) {
		_mage_dirty_row_ptr[i] = 0;
	}
}

void _mage_screen_buffer_clear() {
	for (uint8_t row = 0; row < 40; row++) {
		for (uint8_t col = 0; col < 100; col++) {
			for (uint8_t i = 0; i < 2; i++) {
				_mage_screen_chr[row][col][i] = 0;
			}
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
	_mage_screen_buffer_clear();
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
	if (!_mage_display_off) { avdc_wait_access(); }
	for (_mage_render_buffer_item *item = _mage_render_buffer; item < _mage_render_buffer_ptr; item++) {
		avdc_wait_ready();
		if (item->addr != 0) {
			AVDC_CUR_LWR = LO(item->addr);
			AVDC_CUR_UPR = HI(item->addr);
		}
		AVDC_CHR = item->ch;
		AVDC_ATTR = item->attr;
		AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
	}
	_mage_render_buffer_ptr = _mage_render_buffer;
	_mage_render_buffer_weight = 0;
}

void _mage_write_char(uint8_t row, uint8_t col, bool move_cursor) {
	// _mage_render_buffer_ptr->addr = move_cursor ? (80 + row * 100 + col) : 0;
	// _mage_render_buffer_ptr->attr = AVDC_ATTR_UDG | ((_mage_screen_chr[row][col] & 0xC0) >> 2);
	// if ((_mage_screen_chr[row][col] & 0xC0) == MAGE_ATTR_DIM) {
	// 	_mage_render_buffer_ptr->ch = ~_mage_screen_chr[row][col] & 0x3F;
	// } else {
	// 	_mage_render_buffer_ptr->ch = _mage_screen_chr[row][col] & 0x3F;
	// }
	// _mage_render_buffer_ptr++;
}

void _mage_set_cursor(uint16_t addr) {
	if (!_mage_display_off) { avdc_wait_access(); }
    avdc_wait_ready();
    AVDC_CUR_LWR = LO(addr);
    AVDC_CUR_UPR = HI(addr);
}

// void mage_set_block(uint8_t row, uint8_t col, uint8_t val, uint8_t attr) {
// 	if ((_mage_screen_chr[row][col][0] != val) || ((_mage_screen_chr[row][col][1] & 127) != attr)) {
// 		_mage_screen_chr[row][col][0] = val;
// 		_mage_screen_chr[row][col][1] = attr | 128;
// 	}
// }

void mage_set_block(uint8_t row, uint8_t col, uint8_t val, uint8_t attr) {
	uint16_t addr = row * 100 + col;
	uint8_t *ptr = (uint8_t *)_mage_screen_chr + (addr << 1) + 1;
	if ((*(ptr - 1) != val) || ((*ptr & 63) != attr)) {
		*(ptr - 1) = (attr == MAGE_ATTR_DIM) ? (~val & 63) : val;
		*ptr = attr | 128;
		*(ptr - ((addr & 7) << 1)) |= 64; // dirty block bit
	}
}

void mage_flag_dirty(uint8_t row, uint8_t col) {
	_mage_dirty_row[row >> 3] |= 1 << (row & 7);
	_mage_dirty_char[row][col >> 3] |= 1 << (col & 7);
}
// But multiplying by 100 (and for many other numbers) is also possible:
// a*=100;
// equals
// a= a*64 + a*32 + a*4;
// equals
// a=(a<<6)+(a<<5)+(a<<2);

// void mage_render_background_asm() {
// 	__asm
// 		// HL = addr of mage_screen_buffer
// 		LD HL, #__mage_screen_buffer
// 		// *** loop over rows (B)
// 		LD B, #0
// 	row_repeat:
// 		// *** loop over columns (C)
// 		LD C, #0
// 	col_repeat:
// 		// E = mage_screen_buffer[B][C]
// 		LD A, (HL)
// 		LD E, A
// 		JR Z, skip
// 		// HL = address (B * 100 + C + 80)
// 		PUSH HL
// 		// rows * 100 ...
// 		LD E, B
// 		LD D, #0
// 		LD HL, #0
// 		ADD HL, DE
// 		ADD HL, HL
// 		ADD HL, HL
// 		LD D, H
// 		LD E, L
// 		ADD HL, HL
// 		ADD HL, HL
// 		ADD HL, HL
// 		PUSH HL
// 		ADD HL, DE
// 		LD D, H
// 		LD E, L
// 		POP HL
// 		ADD HL, HL
// 		ADD HL, DE
// 		// ... + cols + 80
// 		LD D, #0
// 		LD E, #80
// 		ADD HL, DE
// 		LD E, C
// 		ADD HL, DE
// 		// D = character
// 		LD A, E
// 		AND #0xC0
// 		SUB #MAGE_ATTR_DIM
// 		JR Z, neg
// 		LD A, E
// 		AND #0x3F
// 		LD D, A
// 		JR char_done
// 	neg:
// 		LD A, E
// 		CPL
// 		AND #0x3F
// 		LD D, A
// 	char_done:
// 		// E = attribute
// 		LD A, E
// 		AND #0xC0
// 		SRL A
// 		SRL A
// 		OR #AVDC_ATTR_UDG
// 		LD E, A
// 		// execute command
// 		CALL _avdc_wait_access
// 		CALL _avdc_wait_ready
// 		LD A, L
// 		OUT (_AVDC_CUR_LWR), A
// 		LD A, H
// 		OUT (_AVDC_CUR_UPR), A
// 		LD A, D
// 		OUT (_AVDC_CHR), A
// 		LD A, E
// 		OUT (_AVDC_ATTR), A
// 		LD A, #AVDC_CMD_WRITE_AT_CUR
// 		OUT (_AVDC_CMD), A
// 		// retore HL
// 		POP HL
// 	skip:
// 		INC C
// 		INC HL
// 		LD A, #100
// 		CP C
// 		JR NZ, col_repeat
// 		// *** end loop over columns
// 		INC B
// 		LD A, #40
// 		CP B
// 		JR NZ, row_repeat
// 		// *** end loop over rows
// 	__endasm;
// }

// Display ON
// * Big change (1649): 930 ms
// * Small change (< 760): 360 ms
// Display OFF
// * Big change:
// * Small change:
// void mage_render_1() {
// 	uint8_t r = 0;
// 	uint8_t c = 0;
// 	_mage_set_cursor(80);
// 	for (uint8_t row = 0; row < 40; row++) {
// 		for (uint8_t col = 0; col < 100; col++) {
// 			uint8_t attr = _mage_screen_attr[row][col];
// 			if ((attr & 128) != 0) {
// 				attr &= 127;
// 				_mage_screen_attr[row][col] = attr;
// 				uint8_t chr = _mage_screen_chr[row][col];
// 				if (attr == MAGE_ATTR_DIM) { chr = ~chr & 63; }
// 				if (!_mage_display_off) { avdc_wait_access(); }
// 				avdc_wait_ready();
// 				if ((row != r) || (col != c)) {
// 					uint16_t addr = row * 100 + col + 80;
// 					AVDC_CUR_LWR = LO(addr);
// 					AVDC_CUR_UPR = HI(addr);
// 				}
// 				AVDC_CHR = chr;
// 				AVDC_ATTR = AVDC_ATTR_UDG | attr;
// 				AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
// 				r = row;
// 				c = col + 1;
// 			}
// 		}
// 	}
// }

// Display ON
// * Big change: 880 ms
// * Small change: 300 ms
// Display OFF
// * Big change:
// * Small change:
// void mage_render_1b() {
// 	uint8_t r = 0;
// 	uint8_t c = 0;
// 	_mage_set_cursor(80);
// 	uint8_t *ptr_chr = (uint8_t *)_mage_screen_chr;
// 	uint8_t *ptr_attr = (uint8_t *)_mage_screen_attr;
// 	for (; ptr_chr < ((uint8_t *)_mage_screen_chr + 4000); ptr_chr++, ptr_attr++) {
// 		if ((*ptr_attr & 128) != 0) {
// 			*ptr_attr &= 127;
// 			uint8_t chr = *ptr_chr;
// 			if (*ptr_attr == MAGE_ATTR_DIM) { chr = ~chr & 63; }
// 			if (!_mage_display_off) { avdc_wait_access(); }
// 			avdc_wait_ready();
// 			//if ((row != r) || (col != c)) {
// 				uint16_t addr = ptr_chr - (uint8_t *)_mage_screen_chr + 80;
// 				AVDC_CUR_LWR = LO(addr);
// 				AVDC_CUR_UPR = HI(addr);
// 			//}
// 			AVDC_CHR = chr;
// 			AVDC_ATTR = AVDC_ATTR_UDG | *ptr_attr;
// 			AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
// 			//r = row;
// 			//c = col + 1;
// 		}
// 	}
// }

// Display ON
// * Big change: 970 ms
// * Small change: 370 ms
// Display OFF
// * Big change:
// * Small change:
// void mage_render_1c() {
// 	uint8_t r = 0;
// 	uint8_t c = 0;
// 	_mage_set_cursor(80);
// 	for (uint16_t addr = 80; addr < 4080; addr++) {
// 		uint8_t *ptr_attr = (uint8_t *)_mage_screen_attr + addr - 80;
// 		if ((*ptr_attr & 128) != 0) {
// 			*ptr_attr &= 127;
// 			uint8_t chr = *((uint8_t *)_mage_screen_chr + addr - 80);
// 			if (*ptr_attr == MAGE_ATTR_DIM) { chr = ~chr & 63; }
// 			if (!_mage_display_off) { avdc_wait_access(); }
// 			avdc_wait_ready();
// 			//if ((row != r) || (col != c)) {
// 				AVDC_CUR_LWR = LO(addr);
// 				AVDC_CUR_UPR = HI(addr);
// 			//}
// 			AVDC_CHR = chr;
// 			AVDC_ATTR = AVDC_ATTR_UDG | *ptr_attr;
// 			AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
// 			//r = row;
// 			//c = col + 1;
// 		}
// 	}
// }

// Display ON
// * Big change: 1020 ms
// * Small change: 450 ms
// Display OFF
// * Big change:
// * Small change:
// void mage_render_2() {
// 	uint8_t r = 0;
// 	uint8_t c = 0;
// 	_mage_set_cursor(80);
// 	for (uint8_t row = 0; row < 40; row++) {
// 		for (uint8_t col = 0; col < 100; col++) {
// 			uint8_t attr = _mage_screen_attr[row][col];
// 			if ((attr & 128) != 0) {
// 				attr &= 127;
// 				_mage_screen_attr[row][col] = attr;
// 				uint8_t chr = _mage_screen_chr[row][col];
// 				if (attr == MAGE_ATTR_DIM) { chr = ~chr & 63; }
// 				uint16_t addr = 0;
// 				uint8_t weight = _MAGE_BUFFER_WRITE_WEIGHT;
// 				if ((row != r) || (col != c)) {
// 					addr = row * 100 + col + 80;
// 					weight = _MAGE_BUFFER_MOVE_AND_WRITE_WEIGHT;
// 				}
// 				if (_mage_render_buffer_weight + weight > _MAGE_BUFFER_WEIGHT_LIMIT) {
// 					_mage_flush();
// 				}
// 				_mage_render_buffer_ptr->addr = addr;
// 				_mage_render_buffer_ptr->attr = AVDC_ATTR_UDG | attr;
// 				_mage_render_buffer_ptr->ch = chr;
// 				_mage_render_buffer_ptr++;
// 				_mage_render_buffer_weight += weight;
// 				r = row;
// 				c = col + 1;
// 			}
// 		}
// 	}
// 	_mage_flush();
// }

// void mage_render_3() {
// 	// set cursor to right after the row table
//     _mage_set_cursor(40 * 2);
// 	uint8_t r = 0;
// 	uint8_t c = 0;
// 	for (uint8_t row_byte = 0; row_byte < 5; row_byte++) {
// 		uint8_t dirty_row_byte = _mage_dirty_row[row_byte];
// 		if (dirty_row_byte != 0) {
// 			for (uint8_t row_bit = 0; row_bit < 8; row_bit++) {
// 				if ((dirty_row_byte & (1 << row_bit)) != 0) {
// 					uint8_t dirty_row = row_byte * 8 + row_bit;
// 					//printf("row: %u \n\r", dirty_row);
// 					for (uint8_t col_byte = 0; col_byte < 13; col_byte++) {
// 						if (_mage_dirty_char[dirty_row][col_byte] != 0) {
// 							uint8_t dirty_char_byte = _mage_dirty_char[dirty_row][col_byte];
// 							for (uint8_t char_bit = 0; char_bit < 8; char_bit++) {
// 								if ((dirty_char_byte & (1 << char_bit)) != 0) {
// 									//printf("%u %u \n\r", dirty_row, col_byte * 8 + char_bit);
// 									uint8_t col = col_byte * 8 + char_bit;
// 									bool move_cursor = dirty_row != r || col != c;
// 									r = dirty_row;
// 									c = col + 1;
// 									uint8_t w = move_cursor ? _MAGE_BUFFER_MOVE_AND_WRITE_WEIGHT : _MAGE_BUFFER_WRITE_WEIGHT;
// 									if (_mage_render_buffer_weight + w > _MAGE_BUFFER_WEIGHT_LIMIT) {
// 										_mage_flush();
// 									}
// 									_mage_write_char(dirty_row, col, move_cursor);
// 									_mage_render_buffer_weight += w;
// 								}
// 							}
// 							_mage_dirty_char[dirty_row][col_byte] = 0; // clear dirty char bits
// 						}
// 					}
// 				}
// 			}
// 			_mage_dirty_row[row_byte] = 0; // clear dirty row bits
// 		}
// 	}
// 	_mage_flush();
// }

// void mage_render() {
// 	_mage_set_cursor(80);
// 	uint8_t *ptr_chr = (uint8_t *)_mage_screen_chr;
// 	uint8_t total_weight = 0;
// 	bool move_cursor = false;
// 	for (uint16_t addr = 80; addr < 4080; ptr_chr += 2, addr++) {
// 		uint8_t *ptr_attr = ptr_chr + 1;
// 		if ((*ptr_attr & 128) != 0) {
// 			*ptr_attr &= 127;
// 			uint8_t w = move_cursor ? _MAGE_BUFFER_MOVE_AND_WRITE_WEIGHT : _MAGE_BUFFER_WRITE_WEIGHT;
// 			total_weight += w;
// 			if (!_mage_display_off && (total_weight > _MAGE_BUFFER_WEIGHT_LIMIT)) {
// 				avdc_wait_access();
// 				total_weight = w;
// 			}
// 			avdc_wait_ready();
// 			if (move_cursor) {
// 				move_cursor = false;
// 				AVDC_CUR_LWR = LO(addr);
// 				AVDC_CUR_UPR = HI(addr);
// 			}
// 			AVDC_CHR = *ptr_chr;
// 			AVDC_ATTR = AVDC_ATTR_UDG | *ptr_attr;
// 			AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
// 		} else {
// 			move_cursor = true;
// 		}
// 	}
// }



void mage_render() {
	uint8_t *ptr = (uint8_t *)_mage_screen_chr + 1;
	uint16_t addr = 80;
jump:
	while ((*ptr & 64) == 0) {
		ptr += 16;
		addr += 8;
		if (addr == 4080) { return; }
	}
walk:
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
	avdc_wait_access();
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
		if ((count & 3) == 0) { 
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
	goto walk;
}

void mage_set_row_ptr(uint8_t row, uint16_t ptr) {
	_mage_row_ptr[row] = ptr;
	_mage_dirty_row_ptr[row >> 3] |= 1 << (row & 7);
}

// WARNME: this could be optimized (see mage_render)
void mage_update_rows() {
    for (uint16_t row_byte = 0; row_byte < 5; row_byte++) {
    	uint8_t dirty_row_byte = _mage_dirty_row[row_byte];
    	if (dirty_row_byte != 0) {
    		for (uint8_t row_bit = 0; row_bit < 8; row_bit++) {
				if ((dirty_row_byte & (1 << row_bit)) != 0) {
					uint8_t row = dirty_row_byte * 8 + row_bit;
					_mage_set_cursor(row * 2);
					// first byte
					AVDC_CHR = LO(_mage_row_ptr[row]);
					AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
					// second byte
					avdc_wait_ready();
					AVDC_CHR = HI(_mage_row_ptr[row]);
					AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
				}
			}
    	}
    }
}