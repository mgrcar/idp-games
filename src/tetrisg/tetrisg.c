#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "tetrisg.h"
#include "../common/avdc.h"
#include "../common/utils.h"

// buffers

uint8_t buffer[BUFFER_SIZE];
uint8_t str_buffer[32];
uint8_t *gfx_index[120];

// state

state game_state;
bool show_next;
uint8_t level;
uint8_t steps;
long score;
uint8_t full_lines;
uint8_t full_lines_add;
long time_passed;

void state_init() {
	game_state = STATE_PLAY;
	show_next = false;
	level = 0;
	steps = 0;
	score = 0;
	full_lines = 0;
	full_lines_add = 0;
	time_passed = 0;
	srand(timer());
}

// key

key key_get() {
	char key;
	if (!(key = kbhit())) { return KEY_NONE; }
	switch (key) {
	case 'a':
	case 'A':
	case '7':
		return KEY_LEFT;
	case 'd':
	case 'D':
	case '9':
		return KEY_RIGHT;
	case 'w':
	case 'W':
	case '8':
		return KEY_ROTATE;
	case 's':
	case 'S':
	case '5':
	case ' ': 
		return KEY_DROP;
	case 'n':
	case 'N':
		return KEY_RESTART;
	case 'p':
	case 'P':
		return KEY_PAUSE;
	case '1':
		return KEY_SHOW_NEXT;
	case 'x':
	case 'X':
		return KEY_EXIT;
	default:
		return KEY_OTHER;
	}
}

// playfield

uint8_t playfield[19][10];

void playfield_init() {
	for (uint8_t i = 0; i < 19; i++) {
		playfield_clear_row(i);
	}
}

void playfield_update_bkgr() {
	for (uint8_t r = 0; r < 4; r++) {
		if (block_pos_y + r >= 0 && block_pos_y + r < 19) {
			for (uint8_t c = 0; c < 4; c++) {
				if (block_pos_x + c >= 0 && block_pos_x + c < 10) {
					if (blocks[block_type][block_rot][r][c] != '0') {
						playfield[block_pos_y + r][block_pos_x + c] = blocks[block_type][block_rot][r][c];
					}
				}
			}
		}
	}
}

void playfield_clear_row(uint8_t row) {
	for (uint8_t j = 0; j < 10; j++) {
		playfield[row][j] = '0';
	}
}

void playfield_move_row(uint8_t row_src, uint8_t row_dst) {
	for (uint8_t j = 0; j < 10; j++) {
		playfield[row_dst][j] = playfield[row_src][j];
	}
	playfield_clear_row(row_src);
}

uint8_t playfield_collapse() {
	uint8_t lines = 0;
	for (int8_t y = 18; y >= 0; y--) {
		bool full_line = true;
		for (uint8_t x = 0; x < 10; x++) {
			if (playfield[y][x] == '0') { full_line = false; break; }
		}
		if (full_line) {
			lines++;
			// clear line y
			playfield_clear_row(y);
			gdp_draw(gfx_playfield_row_empty, 12, 382, 3 + y * 13);
			kbd_beep(false);
			// shift lines
			for (int8_t yy = y - 1; yy >= 0; yy--) {
				// check if yy is empty
				bool done = true;
				for (uint8_t col = 0; col < 10; col++) {
					if (playfield[yy][col] != '0') { done = false; break; }
				}
				// copy line from yy to yy + 1
				playfield_move_row(yy, yy + 1);
				gdp_draw(gfx_playfield_row_empty, 12, 382, 3 + yy * 13);
				render_playfield_row(yy + 1);
			    if (done) { 
					break; 
				}
			}
			y++;
		}
	}
	return lines;
}

// render

void render_init() {
	render_level(); 
}

void render_pause() { 
	_render_popup_blank();
	_render_message("Pavza", "P/N Nadaljuj");
}

void _render_popup_blank() {	
	gdp_draw_rect(GDP_TOOL_ERASER, 394, 107, 236, 39);
	gdp_fill_rect(GDP_TOOL_PEN, 396, 108, 232, 37);
}

void render_clear_pause() { 
	for (uint8_t i = 8; i < 11; i++) {
		gdp_draw(gfx_playfield_row_empty, 12, 382, 3 + i * 13);
		render_playfield_row(i);
	}
	render_block(-50, -50, block_rot);
}

void render_next_block() {
	render_clear_next_block(); 
	for (uint8_t r = 1; r < 3; r++) {
		for (uint8_t c = 0; c < 4; c++) {
			uint8_t block_piece = blocks[block_type_next][0][r][c];
			if (block_piece != '0') {
				_render_block_piece(848 + c * 26, 27 + r * 13, block_piece - '1');
			}
		}
	}
}

void render_clear_next_block() { 
	gdp_fill_rect(GDP_TOOL_PEN, 834, 40, 118, 26); 
}

void render_level() {
	const uint8_t *file_name = "TETBKG00.BIN";
	memcpy(str_buffer, file_name, strlen(file_name) + 1);
	str_buffer[7] = '0' + level;
	_render_background(str_buffer, true);
}

void render_score() { 
	itoa(score, str_buffer, 10);
	gdp_fill_rect(GDP_TOOL_PEN, 838, 11, 114, 7); 
	gdp_set_xy(954 - strlen(str_buffer) * 12, 18);
	gdp_write(GDP_TOOL_ERASER, str_buffer);	
}

void render_full_lines() { 
	itoa(full_lines, str_buffer, 10);
	gdp_fill_rect(GDP_TOOL_PEN, 862, 27, 90, 7); 
	gdp_set_xy(954 - strlen(str_buffer) * 12, 34);
	gdp_write(GDP_TOOL_ERASER, str_buffer);	
}

void render_game_over() { 
	_render_popup_blank();
	_render_message("Konec igre", "N Nova  X Izhod");
}

void render_clear_block() {  
	for (uint8_t r = 0; r < 4; r++) {
		if (block_pos_y + r >= 0 && block_pos_y + r < 19) {
			for (uint8_t c = 0; c < 4; c++) {
				if (block_pos_x + c >= 0 && block_pos_x + c < 10) {
					if (blocks[block_type][block_rot][r][c] != '0') {
						uint16_t x = 382 + (block_pos_x + c) * 26;
						uint8_t y = 3 + (block_pos_y + r) * 13;
						_render_block_piece_fast(x, y, GDP_TOOL_PEN);
					}
				}
			}
		}
	}
}

void render_block(int8_t dx, int8_t dy, uint8_t rot_prev) {  
	for (int8_t r = -1; r < 4; r++) {
		if (block_pos_y + r >= 0 && block_pos_y + r < 19) {
			for (int8_t c = -1; c < 5; c++) {
				if (block_pos_x + c >= 0 && block_pos_x + c < 10) {
					uint16_t x = 382 + (block_pos_x + c) * 26;
					uint8_t y = 3 + (block_pos_y + r) * 13;
					uint8_t current = _block_get_piece(block_rot, r, c);
					uint8_t prev = _block_get_piece(rot_prev, r - dy, c - dx);
					if (current != prev) {
						if (current == '0') { // current == '0' && prev != '0'
							// delete block
							_render_block_piece_fast(x, y, GDP_TOOL_PEN);
						} else if (prev == '0') { // current != '0' && prev == '0'
							// draw block
							_render_block_piece_fast(x, y, GDP_TOOL_ERASER);
						}
					}
				}
			}
		}
	}
}


void render_block_fallen() {  
	for (uint8_t r = 0; r < 4; r++) {
		if (block_pos_y + r >= 0 && block_pos_y + r < 19) {
			for (uint8_t c = 0; c < 4; c++) {
				if (block_pos_x + c >= 0 && block_pos_x + c < 10) {
					if (blocks[block_type][block_rot][r][c] != '0') {
						uint16_t x = 382 + (block_pos_x + c) * 26;
						uint8_t y = 3 + (block_pos_y + r) * 13;
						_render_block_piece(x, y, block_type);
					}
				}
			}
		}
	}
}

void render_playfield_row(uint8_t row) {
	for (uint8_t j = 0; j < 10; j++) {
		uint8_t playfield_cell = playfield[row][j];
		if (playfield_cell != '0') {
			_render_playfield_cell(
				382 + j * 26, 3 + row * 13,
				playfield_cell - '1'
			);					
		}
	}	
}

// gfx

uint8_t *gfx_popup_loader[] = {
	"\x04\x00\x3F\x00\x37",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x08\x81\x1E\x8B\x01\x3F\x00\x12\x01",
	"\x08\x81\x1E\x8B\x01\x3F\x00\x12\x01",
	"\x0A\x81\x1E\x92\x01\x08\x02\x0B\x03\x39\x01",
	"\x0D\x81\x1E\xCD\xC2\x8D\x8C\x94\x8A\x8A\x93\x04\x2E\x01",
	"\x0E\x81\x1E\x8A\x94\x8C\x8E\x8B\x8A\x8C\x8A\xCD\x00\x2C\x01",
	"\x0D\x81\x1E\x8B\xCA\x9C\x8B\xA4\xDA\x9A\xCD\x00\x2C\x01",
	"\x12\x81\x1E\x8B\xCB\xC3\x83\x8B\x8A\x8E\xCB\xC3\xC7\xC2\x8C\x94\x02\x1D\x01",
	"\x0F\x81\x1E\x8B\xCA\xA3\x92\xAD\xCA\xE7\xC2\x8C\x94\x02\x1D\x01",
	"\x05\x81\x37\x03\x3A\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x07\x81\x12\x3F\x00\x11\x12\x01",
	"\x07\x81\x12\x3F\x00\x11\x12\x01",
	"\x07\x81\x12\x3F\x00\x11\x12\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x05\x81\x3F\x00\x35\x01",
	"\x04\x00\x3F\x00\x37"
};

uint8_t *gfx_popup_area_title[] = {
	"\x17\x81\xD8\xD7\xF6\xD1\x02\x08\xD1\x99\xE1\x48\xD5\x82\x8A\x93\x8C\xCA\x8A\xCA\xCA\x8A\x8A\x14",
	"\x13\x84\x9A\xD1\x9C\x48\x8A\xD2\xD2\x92\xCD\xC4\xCC\x15\x94\x95\xD3\x99\x42\x10",
	"\x18\x82\xD2\xE1\x92\xD8\xD4\xD2\x02\x08\xD1\x99\xE1\x48\x9B\x8B\x8B\x8B\x8C\xE2\xDA\xA3\xDA\x01\x0D",
	"\x1A\x81\xD1\xE8\xD8\xD8\xD1\x91\xD1\xD2\xF6\x91\x44\x0A\x92\x9A\xD1\x9A\xD1\x92\xD2\xD8\xD3\x92\x91\x42\x10",
	"\x1B\x86\xD8\xE8\xD8\xD8\xD2\xD2\xD4\xD2\xD1\x99\x9A\xF8\x91\xA1\x91\xA1\x91\xA1\x9B\x96\xD5\xD1\xD2\xD1\x92\x02",
	"\x17\xD8\xE1\x95\xA1\xD2\xD2\xD2\xD2\xD2\x91\xE8\xE1\xD1\x9D\xD6\xD6\xE3\x9D\x42\x08\xD5\xD8\x43",
	"\x1D\x81\xA3\x92\xD8\xA1\x92\xD2\xD4\xD2\xD1\x9A\x92\xE3\xD1\x99\x99\x99\x99\x9A\x91\x92\xD8\xD1\xD1\xD3\xD3\xD1\xE1\x43",
	"\x1B\x81\xA3\x92\xD8\xA1\x96\xF6\x91\x92\x91\xD1\xE1\xD8\x93\xD8\x93\xD8\x93\xD8\xD1\xE2\xD1\x02\x08\xD1\xE1\x43",
	"\x1C\x81\xA3\x92\xD8\xA1\x92\xD2\xD4\xD2\xD1\x03\x08\xD1\xD1\xD8\x92\x91\x99\x99\x9A\xD1\x9B\x9D\xD2\xD3\xD1\xE1\x43",
	"\x1A\x81\xA3\x92\xD8\xA1\x92\xD2\xD2\xD2\xD2\x91\x92\x91\xD1\xE1\x42\x10\x42\x0A\xD3\xD2\xD1\xD5\xD1\xE1\x43",
	"\x19\x81\xA1\xD8\xE2\xA1\xD1\xD2\x9C\xD2\xD1\x9B\xD1\xE7\x9A\x91\x99\x92\x9A\x93\x94\xE3\x44\x08\xD1\x43",
	"\x16\x81\x9A\xD8\x95\x94\x93\xA2\xD5\x91\x92\x91\xD1\xD1\xE1\xE1\xD1\x9A\xD1\x44\x0C\x02\x17",
	"\x1B\x82\xD1\xD8\xA2\x92\xD1\xA9\x9B\xD3\xD1\x9B\xD1\xE3\xD1\xA1\x91\x9A\x91\xA1\xD1\xD2\xE1\xD1\x92\xE1\xE5\x04",
	"\x11\x00\x21\xD1\xD2\x91\x92\x91\xD1\xE7\xD6\x42\x0A\x95\xD5\xE3\xE2\x9C",
	"\x14\x00\x1B\x94\xD3\xD1\x9B\xD1\xE1\x94\x91\x92\xE8\xD3\x91\xD1\xD4\x94\x9B\x42\x0E",
	"\x17\x1B\xD2\xD2\xAA\x8A\x92\xCA\xCD\xC3\x82\xDB\x82\xDB\xC2\xDA\xCC\xD3\xC2\x8A\xEA\x9A\xD3\x82",
	"\x15\x00\x1D\xD2\xD3\xD1\x03\x0A\xD8\x91\x92\x92\xE1\x91\x92\xD6\x93\x97\xD3\xD8\xD1\x45",
	"\x19\x91\x99\xD8\x94\xD2\xD2\xD3\xD2\xD1\xD2\x91\x92\x91\xD4\x44\x0E\xD7\xD2\x95\x94\xD2\xD3\x92\xD8\x02",
	"\x1C\x08\x8B\xD2\xD2\xD2\xD2\xCA\xD2\xDA\x8B\x92\xD6\xCA\xCA\x9A\x92\x93\x8A\x92\xD2\xCA\xCA\xCB\xC2\xDA\xDB\xC2\xCD",
	"\x1C\x81\x99\x99\x9C\xD2\xD2\xD3\xD8\x96\x91\x92\x91\xD1\xE2\x92\xD1\xD1\xE6\xD1\xD8\xD4\xD3\x99\xD2\xD3\x92\xD8\x02",
	"\x17\x91\xA9\x91\xE2\xD2\xD2\xD1\xF6\x9A\x92\x48\x94\x8A\xCB\xC2\x9B\xF2\x9A\xE2\xE2\xDB\xC2\xCD",
	"\x18\x81\xD8\x91\xD8\xE2\xD2\xD7\xD5\xD2\x42\x0D\xE2\xD1\xD6\x42\x0E\xD1\xD8\xD4\xD2\xD3\x92\xD8\x02",
	"\x18\x81\xD8\xA1\x93\xD2\xD2\xD2\xD1\xD8\xD6\xA9\x05\x0B\xA9\x9B\xE3\xD4\xE3\x92\xD2\xD3\xD8\xD1\x45",
	"\x1B\x82\xD1\x99\x95\xD2\xD2\xD3\xD8\xE2\xD8\xA1\xA9\xD2\xD4\x99\xD8\xE1\x91\x91\xD4\xA9\x96\xD2\xD3\x92\xD8\x02",
	"\x17\x82\xD8\x99\x93\xD2\xD2\xD2\xD1\xD8\x96\x0B\xDA\xD2\xDA\xE2\xE2\xD4\xE2\xD4\xDA\xFB\xC2\xCD",
	"\x16\xD2\x91\xF3\xD2\xD2\xD3\xD1\xD2\x42\x16\xD3\x9B\x94\x9C\xD2\xD8\xD3\xD2\xD2\xD7\xD8\x02",
	"\x18\xE8\x99\xE5\xD2\xD2\xD1\xE6\xD8\x91\x99\xD1\x07\x09\x92\xE2\x94\x94\x92\xD2\xD2\xD3\xD8\x91\x45",
	"\x11\xE1\x99\xD1\xD2\xD2\xD2\xD3\xE1\xD2\xA1\x99\x03\x32\xD6\x91\xE8\x02",
	"\x11\x81\x92\x99\xD7\xD6\xD1\xE6\xD8\x91\x99\xD3\xD2\xD2\x42\x2B\xD8\x96",
	"\x1B\x81\xD2\xF1\xD2\xD2\xD2\xD3\xD1\xD2\xD1\x99\x99\x9A\xD2\xD2\xD4\xD8\xA1\x91\xD1\xA9\xD1\x03\x10\x91\x92\x43",
	"\x17\x81\x99\x9A\xD6\x42\x09\xE6\xD8\x91\x99\xD3\xD2\xD2\xD2\x02\x13\xAA\xD2\xD2\xD3\xD8\x92\x44",
	"\x16\x81\x99\x9C\xD2\xD2\xD2\xD3\xE1\xD2\xA1\x99\x9A\xD2\xD2\x42\x22\xD2\xD2\xD2\x91\x92\x43",
	"\x18\x81\x99\x03\x0A\x42\x09\xE6\xD8\x91\x99\xD3\xD2\xD2\xD1\xA9\x09\xCB\x00\x08\xD2\xF2\xDB\x82\xD4",
	"\x16\x91\xD2\xD3\xD2\xD2\xD2\xD3\xD8\xD2\xD1\x99\x99\x93\xD2\xD2\x42\x22\x42\x0A\x91\x92\x43",
	"\x17\xA2\x92\xD2\xD2\x42\x09\xE5\x92\x91\x9A\xD2\xD2\xD2\xD2\xA1\x0B\x01\x08\xDA\x0B\xD8\x92\x44",
	"\x0F\x81\x9A\x94\xD6\xD2\x42\x08\x42\x3F\x81\xD2\xD2\x91\x92\x43",
	"\x1A\xA9\x92\xD2\xD2\xD6\xD1\xE6\xD1\x92\x92\xD2\xD2\xD2\xD1\xA9\xD8\x91\x99\xD3\xD2\x42\x0A\xD3\xD8\x92\x44",
	"\x1C\x91\x92\xD3\xD6\xD2\xD3\xD8\xD2\xD1\x92\x91\x94\xD2\xD2\xD3\xD8\x91\x99\x99\x9A\xD2\xD6\xD2\xD2\xD2\x91\x92\x43",
	"\x1A\xAA\xD1\xD2\xD2\xD6\xD1\x44\x09\x92\x92\xD2\xD2\xD2\xD1\xA9\xD8\x91\x99\xD3\xD2\xD2\xD6\xD3\xD8\x92\x44"
};

uint8_t *gfx_playfield_row_empty[] = {
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x13\xB1\x0C\x01\x0C\x01\x0C\x01\x0C\x01\x0C\x01\x0C\x01\x0C\x01\x0C\x01\x0C\x8E",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84",
	"\x04\x3F\x00\x3F\x84"
};

uint8_t *gfx_start_button_pressed[] = {
	"\x02\x3F\x01",
	"\x02\x3E\x03",
	"\x02\x3D\x05",
	"\x02\x3C\x07",
	"\x02\x3B\x09",
	"\x02\x3A\x0B",
	"\x02\x39\x0D",
	"\x02\x38\x0F",
	"\x02\x37\x11",
	"\x02\x36\x13",
	"\x02\x35\x15",
	"\x02\x34\x17",
	"\x02\x33\x19",
	"\x02\x32\x1B",
	"\x02\x31\x1D",
	"\x02\x30\x1F",
	"\x02\x2F\x21",
	"\x02\x00\x7F",
	"\x02\x02\x7B",
	"\x02\x04\x77",
	"\x02\x06\x73",
	"\x02\x08\x6F",
	"\x02\x0A\x6B",
	"\x02\x0C\x67",
	"\x02\x0E\x63",
	"\x02\x10\x5F",
	"\x02\x12\x5B",
	"\x02\x14\x57",
	"\x02\x16\x53",
	"\x02\x18\x4F",
	"\x02\x1A\x4B",
	"\x02\x1C\x47",
	"\x02\x1E\x43",
	"\x02\x20\x3F",
	"\x02\x22\x3B",
	"\x02\x23\x39",
	"\x02\x22\x3B",
	"\x02\x21\x3D",
	"\x02\x20\x3F",
	"\x02\x1F\x41",
	"\x04\x1E\x20\x03\x20",
	"\x04\x1D\x1E\x09\x1E",
	"\x04\x1C\x1C\x0F\x1C",
	"\x04\x1B\x1A\x15\x1A",
	"\x04\x1A\x17\x1D\x17",
	"\x04\x19\x15\x23\x15",
	"\x04\x18\x13\x29\x13",
	"\x04\x17\x11\x2F\x11",
	"\x04\x16\x0E\x37\x0E",
	"\x04\x15\x0C\x3D\x0C",
	"\x04\x14\x0A\x43\x0A",
	"\x04\x13\x07\x4B\x07",
	"\x04\x12\x05\x51\x05",
	"\x04\x11\x03\x57\x03",
	"\x04\x10\x01\x5D\x01"
};

uint8_t *gfx_blocks[][7] = {
	{
		// big balls
		"\x03\x82\xD2\x01",
		"\x02\x91\x92",
		"\x01\x95",
		"\x01\x95",
		"\x03\x82\xD2\x01",
		"\x02\xD1\x99",
		"\x02\x83\x99"
	}, {
		// renault
		"\x02\x83\x99",
		"\x03\x82\xD8\x02",
		"\x03\x81\x99\x02",
		"\x02\x9B\x01",
		"\x01\x95",
		"\x02\xE2\x01",
		"\x03\x81\x99\x02"
	}, {
		// little balls
		"\x01\x47",
		"\x03\x81\x99\x02",
		"\x01\x07",
		"\x03\x81\x99\x02",
		"\x01\x47",
		"\x03\x81\x99\x02",
		"\x01\x07"
	}, {
		// bricks
		"\x01\x87",
		"\x03\x81\x99\x02",
		"\x01\x87",
		"\x02\x91\x99",
		"\x01\x87",
		"\x03\x81\x99\x02",
		"\x01\x87"
	}, {
		// hash
		"\x02\x83\x44",
		"\x01\x07",
		"\x03\x81\xD2\x42",
		"\x02\x83\x44",
		"\x02\x81\x46",
		"\x02\xA9\x01",
		"\x03\x81\xD2\x42"
	}, { 
		// pattern light
		"\x02\x91\x99",
		"\x03\x81\x99\x02",
		"\x02\x91\x99",
		"\x03\x81\x99\x02",
		"\x02\x91\x99",
		"\x03\x81\x99\x02",
		"\x02\x91\x99"
	}, {
		// pattern dark
		"\x02\x81\x46",
		"\x01\x47",
		"\x02\x81\x46",
		"\x01\x47",
		"\x02\x81\x46",
		"\x01\x47",
		"\x02\x81\x46"
	}
};

// block

p_uint8_t blocks[][4][4] = {
	{
		{ "0000","0111","0100","0000" },
		{ "0010","0010","0011","0000" },
		{ "0001","0111","0000","0000" },
		{ "0110","0010","0010","0000" }
	},
	{
		{ "0000","2222","0000","0000" },
		{ "0020","0020","0020","0020" },
		{ "0000","2222","0000","0000" },
		{ "0020","0020","0020","0020" }
	},
	{
		{ "0000","0333","0030","0000" },
		{ "0030","0033","0030","0000" },
		{ "0030","0333","0000","0000" },
		{ "0030","0330","0030","0000" }
	},
	{
		{ "0000","0044","0440","0000" },
		{ "0040","0044","0004","0000" },
		{ "0000","0044","0440","0000" },
		{ "0040","0044","0004","0000" }
	},
	{
		{ "0000","0550","0055","0000" },
		{ "0005","0055","0050","0000" },
		{ "0000","0550","0055","0000" },
		{ "0005","0055","0050","0000" }
	},
	{
		{ "0000","0660","0660","0000" },
		{ "0000","0660","0660","0000" },
		{ "0000","0660","0660","0000" },
		{ "0000","0660","0660","0000" }
	},
	{
		{ "0000","0777","0007","0000" },
		{ "0077","0070","0070","0000" },
		{ "0700","0777","0000","0000" },
		{ "0070","0070","0770","0000" }
	}
};

int8_t block_type;
uint8_t block_rot;
int8_t block_pos_x;
int8_t block_pos_y;

int8_t block_type_next;

void block_init() {
	block_type = -1;
	block_rot = 0;
	block_pos_x = 3;
	block_pos_y = -1;

	block_type_next = -1;
}

bool block_move_left() {
	if (block_check(block_pos_x - 1, block_pos_y, block_rot)) {
		block_pos_x--;
		render_block(1, 0, block_rot);
		return true;
	}
	return false;
}

bool block_move_right() {
	if (block_check(block_pos_x + 1, block_pos_y, block_rot)) {
		block_pos_x++;
		render_block(-1, 0, block_rot);
		return true;
	}
	return false;
}

bool block_check(int8_t pos_x, int8_t pos_y, uint8_t rot) {
	for (int8_t block_y = 0, grid_y = pos_y; block_y < 4; block_y++, grid_y++) {
		for (int8_t block_x = 0, grid_x = pos_x; block_x < 4; block_x++, grid_x++) {
			if (blocks[block_type][rot][block_y][block_x] != '0' && 
				(grid_x < 0 || grid_y < 0 || grid_x >= 10 || grid_y >= 19 || playfield[grid_y][grid_x] != '0')) {
				return false;
			}
		}
	}
	return true;
}

bool block_move_down() {
	if (block_check(block_pos_x, block_pos_y + 1, block_rot)) {
		block_pos_y++;
		render_block(0, -1, block_rot);
		return true;
	}
	return false;
}

bool block_rotate() {
	uint8_t rot = (block_rot + 1) % 4;
	if (block_check(block_pos_x, block_pos_y, rot)) {
		uint8_t rot_prev = block_rot;
		block_rot = rot;
		render_block(0, 0, rot_prev);
		return true;
	}
	return false;
}

bool block_drop() {
	bool success = block_check(block_pos_x, block_pos_y + 1, block_rot);
	if (success) {
		render_clear_block();
		int8_t dy = 0;
		while (block_check(block_pos_x, block_pos_y + 1, block_rot)) { block_pos_y++; }
		render_block(-50, -50, block_rot);
	}
	return success;
}

bool block_next() {
	if (block_type_next == -1) {
		block_type_next = sys_rand() % 7;
	}
	block_type = block_type_next;
	block_rot = 0;
	block_pos_x = 3;
	block_pos_y = -1;
	block_type_next = sys_rand() % 7;
	bool success = block_check(block_pos_x, block_pos_y, block_rot);
	render_block(-50, -50, block_rot); 
	if (success && show_next) { render_next_block(); } 
	return success;
}

// timer

bool timer_done() {
	long span = (10 - level) * 5;
	return timer_diff() >= span;
}

// game

void game_init() {
	state_init();

	_render_background("TETBKGM.BIN", false);

	gdp_set_mode(GDP_MODE_XOR);

	char key;
	do {
		key = kbhit();
		if (key >= '0' && key <= '9') {
			if (key - '0' != level) {
				if (level <= 4) {
					gdp_fill_rect(GDP_TOOL_PEN, (354 + level * 64), 20, 62, 31);
				} else {
					gdp_fill_rect(GDP_TOOL_PEN, (354 + (level - 5) * 64), 52, 62, 31);
				}
				level = key - '0';
				full_lines_add = level * 10;
				if (level <= 4) {
					gdp_fill_rect(GDP_TOOL_PEN, (354 + level * 64), 20, 62, 31);
				} else {
					gdp_fill_rect(GDP_TOOL_PEN, (354 + (level - 5) * 64), 52, 62, 31);
				}
			}
		}
	} while (key != 13);
	
	gdp_draw_xor_mask(gfx_start_button_pressed, 54, 328, 140);

	gdp_set_mode(GDP_MODE_NORMAL);

	playfield_init();
	block_init();
	render_init();
	block_next();
	timer_reset(0); // resetting the timer should be the last thing here 
}

bool game_play() {
	key key = key_get();
	if (game_state == STATE_PLAY) {
		switch (key) {
		case KEY_LEFT:
			block_move_left();
			break;
		case KEY_RIGHT:
			block_move_right();
			break;
		case KEY_ROTATE:
			block_rotate();
			break;
		case KEY_DROP:
			block_drop();
			break;
		case KEY_PAUSE: 
			{
				time_passed = timer_diff();
				render_pause();
				game_state = STATE_PAUSE;
			}
			break;
		case KEY_SHOW_NEXT:
			show_next = !show_next;
			if (show_next) { render_next_block(); }
			else { render_clear_next_block(); }
			break;
		}
		if (timer_done() && game_state == STATE_PLAY) { 
			if (!block_move_down()) {
				render_block_fallen();
				long points = (21 + 3 * (level + 1)) - steps;
				score += points;
				if (score > 99999) { score = 99999; } // prevent overflow
				render_score();
				steps = 0;
				playfield_update_bkgr();
				full_lines += playfield_collapse();
				if (full_lines + full_lines_add > 99) { full_lines = 99 - full_lines_add; } // prevent overflow
				render_full_lines();
				uint8_t computed_level = ((full_lines + full_lines_add) - 1) / 10;
				if (computed_level > level) {
					level = computed_level;
					kbd_beep(true);
					render_level();
				}
				if (!block_next()) {
					render_game_over();
					game_state = STATE_GAME_OVER;
					return true;
				}
			}
			else { steps++; }
			timer_reset(0);
		}
	} else if (game_state == STATE_PAUSE) {
		if (key == KEY_RESTART || key == KEY_PAUSE) {
			render_clear_pause(); 
			timer_reset(-time_passed);
			game_state = STATE_PLAY;
		} else if (key == KEY_EXIT) {
			return false;
		}
	}
	else if (game_state == STATE_GAME_OVER) {
		if (key == KEY_RESTART) {
			game_init();
		} else if (key == KEY_EXIT) {
			return false;
		}
	}
	return true;
}

// utils

void _render_popup_area(uint8_t **gfx) {
	gdp_draw(gfx, 38, 394, 107);
}

void _render_loader() {
	_render_popup_area(gfx_popup_loader);
}

void _render_loader_bar(uint16_t val, uint16_t max) {
	uint8_t bar = (long)val * 78 / max;
	gdp_set_xy(434, 134);
	gdp_line_dx_pos(GDP_TOOL_PEN, GDP_STYLE_NORMAL, bar * 2 - 1);
}

void _render_block_piece_fast(uint16_t x, uint8_t y, gdp_tool tool) {
	gdp_draw_rect(tool, x, y, 26, 13);
	gdp_draw_rect(tool, x + 4, y + 2, 18, 9);
}

void _render_block_piece(uint16_t x, uint8_t y, uint8_t block_type) { 
	gdp_draw_rect(GDP_TOOL_ERASER, x, y, 26, 13);
	gdp_draw_rect(GDP_TOOL_ERASER, x + 4, y + 2, 18, 9);
	gdp_draw(gfx_blocks[block_type], 6, x + 6, y + 3); 
}

void _render_playfield_cell(uint16_t x, uint8_t y, uint8_t block_type) { 
	_render_block_piece(x, y, block_type);
}

void _render_background_title() {
	int fd = open("TETBKGT0.BIN", O_RDONLY);
	ssize_t read_size = 0;
	size_t tail = 0;
	uint16_t y = 0;
	_render_loader();
	gdp_set_write_page((gdp_write_page + 1) & 1);
	do {
		read_size = read(fd, buffer + tail, BUFFER_SIZE - tail);
		uint8_t *row = buffer;
		uint8_t *eod = buffer + tail + read_size;
		do {
	    	gdp_set_xy(0, y++);
	        gdp_draw_row(row); 
	     	row += row[0] + 1;
	    } while (y <= 255 && row < eod && row + row[0] + 1 <= eod);
    	gdp_set_write_page((gdp_write_page + 1) & 1);
    	_render_loader_bar(y, 512);
    	gdp_set_write_page((gdp_write_page + 1) & 1);
	    tail = eod - row;
	    if (tail > 0) { // WARNME: do I need this?
	    	memcpy(buffer, row, tail);
		}
	} while (y <= 255);
	close(fd);
	_render_loader();
	_render_loader_bar(256, 512); 
	gdp_set_display_page(gdp_write_page);
	fd = open("TETBKGT2.BIN", O_RDONLY);
	read_size = 0;
	tail = 0;
	y = 0;
	gdp_set_write_page((gdp_write_page + 1) & 1);
	do {
		read_size = read(fd, buffer + tail, BUFFER_SIZE - tail);
		uint8_t *row = buffer;
		uint8_t *eod = buffer + tail + read_size;
		do {
	    	gdp_set_xy(0, y++);
	        gdp_draw_row(row); 
	     	row += row[0] + 1;
	    } while (y <= 255 && row < eod && row + row[0] + 1 <= eod);
    	gdp_set_write_page((gdp_write_page + 1) & 1);
    	_render_loader_bar(256 + y, 512);
    	gdp_set_write_page((gdp_write_page + 1) & 1);
	    tail = eod - row;
	    if (tail > 0) { // WARNME: do I need this?
	    	memcpy(buffer, row, tail);
		}
	} while (y <= 255);
	close(fd);
	gdp_set_write_page((gdp_write_page + 1) & 1);
	_render_popup_area(gfx_popup_area_title);
}

void _render_background(uint8_t *file_name, bool render_playfield) {
	int fd = open(file_name, O_RDONLY);
	ssize_t read_size = 0;
	size_t tail = 0;
	uint16_t y = 0;

	_render_loader();
	gdp_set_write_page((gdp_write_page + 1) & 1);

	do {
		read_size = read(fd, buffer + tail, BUFFER_SIZE - tail);
		uint8_t *row = buffer;
		uint8_t *eod = buffer + tail + read_size;
		do {
	    	gdp_set_xy(0, y++);
	        gdp_draw_row(row); 
	     	row += row[0] + 1;
	    } while (y <= 255 && row < eod && row + row[0] + 1 <= eod);
	   	gdp_set_write_page((gdp_write_page + 1) & 1);
	  	_render_loader_bar(y, 256);
	   	gdp_set_write_page((gdp_write_page + 1) & 1);
	    tail = eod - row;
	    if (tail > 0) { // WARNME: do I need this?
	    	memcpy(buffer, row, tail);
		}
	} while (y <= 255);
	close(fd);

	if (render_playfield) {
		for (uint8_t i = 0; i < 19; i++) {
			render_playfield_row(i);
		}
	}

	gdp_set_display_page(gdp_write_page);
}

void _render_message(uint8_t *text_1, uint8_t *text_2) {
	uint8_t str_len = strlen(text_1);
	uint8_t str_width = str_len * 5 + (str_len - 1);
	gdp_set_xy(512 - str_width, 123);
	gdp_write(GDP_TOOL_ERASER, text_1);
	str_len = strlen(text_2);
	str_width = str_len * 5 + (str_len - 1);
	gdp_set_xy(512 - str_width, 137);
	gdp_write(GDP_TOOL_ERASER, text_2);	
}

void _render_title() {
	_render_background_title();
	_gfx_load("TETBKGT1.BIN", 76);
	for (uint8_t i = 1; i <= 38; i++) {
		msleep(i * i / 10 + 1);
		gdp_draw(&gfx_index[76 - 2 * i], 1, 0, 254);
		GDP_SCROLL = 2 * i;
	}
	msleep(1000);
	gdp_set_display_page((gdp_display_page + 1) & 1);
	gdp_set_write_page(gdp_display_page);
	timer_reset(0);
	while (!kbhit() && timer_diff() <= 800);
}

uint8_t _block_get_piece(uint8_t rot, int8_t r, int8_t c) {
	if (r > 3 || r < 0 || c > 3 || c < 0) { return '0'; }
	return blocks[block_type][rot][r][c];
}

void _gfx_load(uint8_t *file_name, uint8_t height) { // WARNME: can only read up to BUFFER_SIZE, and up to 120 rows/lines
	int fd = open(file_name, O_RDONLY);
	read(fd, buffer, BUFFER_SIZE);
	close(fd);
	uint8_t *ptr = buffer;
	for (uint8_t i = 0; i < height; i++) {
		gfx_index[i] = ptr;
		ptr += ptr[0] + 1;
	}
}

// main game loop

int main() {
	gdp_init();
	avdc_init();

	_render_title();

	game_init();
	while (game_play());

	avdc_done();
	gdp_done();

	return 0;
}