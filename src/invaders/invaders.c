#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "invaders.h"
#include "../common/avdc.h"
#include "../common/utils.h"
#include "../common/gdp.h"

#define BUFFER_SIZE 3072

typedef struct {
	uint16_t x_min;
	uint16_t x_max;
	uint8_t invader_count;
} grid_col;

typedef struct {
	grid_col cols[11];
	grid_col cols_new[11];
	uint8_t col_non_empty_first;
	uint8_t col_non_empty_last;
} grid;

typedef enum {
	STATE_MOVE_RIGHT = 0,
	STATE_MOVE_LEFT  = 1,
	STATE_HOLD       = 2
} object_state;

typedef enum {
	GAME_STATE_EXIT,
	GAME_STATE_LEVEL_CLEARED,
	GAME_STATE_GAME_OVER
} game_state;

typedef enum {
	ACTION_NONE     = 0,
	ACTION_EXIT     = 1,
	ACTION_CONTINUE = 2
} user_action;

typedef struct _invader {
	uint16_t x;
	uint8_t y;
	uint8_t col;
	uint8_t type;
	uint8_t dx;
	uint8_t width;
	uint8_t frame;
	object_state state;
	bool destroyed;
	struct _invader *next;
	struct _invader *prev;
} invader;

typedef struct {
	uint16_t x;
	uint8_t wait_frame;
	object_state state;
	uint8_t spawn_delay_sec;
} mothership;

typedef struct {
	object_state state;
	uint16_t x;
} player;

typedef struct {
	uint16_t x;
	uint8_t bits[16][3]; // 16 rows, 3 bytes per row
} shield;

typedef struct {
	uint16_t x;
	uint8_t y;
	uint8_t explode_frame;
	// collision info
	invader *hit_invader; 
	bool hit_mothership;
	shield *hit_shield;
	uint8_t hit_shield_y;
} missle;

uint8_t buffer[BUFFER_SIZE];

invader invaders[55];
player player;
mothership mothership;
grid grid;
missle missle;
shield shields[4];

invader *first_inv;

uint16_t score;
uint8_t lives;
uint8_t level;
uint8_t credits = 1;
uint16_t hi_score = 0;

uint8_t bits_shield_damage_player[] = { 
	137,
	34,
	126,
	255,
	255,
	126,
	36,
	145
};

uint8_t shield_bits[][3] = {	
	{ 15,  255, 192 },
	{ 31,  255, 224 },
	{ 63,  255, 240 },
	{ 127, 255, 248 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 255, 255, 252 },
	{ 254, 1,   252 },
	{ 252, 0,   252 },
	{ 248, 0,   124 },
	{ 248, 0,   124 }
};

// [type][state][frame][scanline]
uint8_t *gfx_invaders[][2][2][8] = {
	{ // type 0
		{ // move right (0)
			{ // frame 0
				"\x03\x89\xC2\x83",
				"\x03\x88\xC4\x82",
				"\x03\x87\xC6\x81",
				"\x06\x86\xC2\x81\xC2\x81\xC2",
				"\x02\x86\xC8",
				"\x05\x88\xC1\x82\xC1\x82",
				"\x07\x87\xC1\x81\xC2\x81\xC1\x81",
				"\x08\x86\xC1\x81\xC1\x82\xC1\x81\xC1"
			}, { // frame 1
				"\x03\x89\xC2\x83",
				"\x03\x88\xC4\x82",
				"\x03\x87\xC6\x81",
				"\x06\x86\xC2\x81\xC2\x81\xC2",
				"\x02\x86\xC8",
				"\x07\x87\xC1\x81\xC2\x81\xC1\x81",
				"\x04\x86\xC1\x86\xC1",
				"\x05\x87\xC1\x84\xC1\x81"
			}
		}, { // move left (1)
			{ // frame 0
				"\x04\x04\x81\xC2\x89",
				"\x03\x04\xC4\x88",
				"\x03\x03\xC6\x87",
				"\x07\x02\xC2\x81\xC2\x81\xC2\x86",
				"\x03\x02\xC8\x86",
				"\x05\x04\xC1\x82\xC1\x88",
				"\x07\x03\xC1\x81\xC2\x81\xC1\x87",
				"\x09\x02\xC1\x01\xC1\x82\xC1\x81\xC1\x86"
			}, { // frame 1
				"\x04\x04\x81\xC2\x89",
				"\x03\x04\xC4\x88",
				"\x03\x03\xC6\x87",
				"\x07\x02\xC2\x81\xC2\x81\xC2\x86",
				"\x03\x02\xC8\x86",
				"\x07\x03\xC1\x81\xC2\x81\xC1\x87",
				"\x06\x02\xC1\x01\x85\xC1\x86",
				"\x05\x03\xC1\x84\xC1\x87"
			}
		}
	}, { // type 1
		{ // move right (0)
			{ // frame 0
				"\x04\x87\xC1\x85\xC1",
				"\x09\x85\xC1\x82\xC1\x83\xC1\x81\x01\xC1",
				"\x06\x85\xC1\x81\xC7\x01\xC1",
				"\x06\x85\xC3\x81\xC3\x81\xC3",
				"\x02\x85\xCB",
				"\x02\x86\xC9",
				"\x04\x87\xC1\x85\xC1",
				"\x04\x86\xC1\x87\xC1"
			}, { // frame 1
				"\x04\x87\xC1\x85\xC1",
				"\x05\x88\xC1\x83\xC1\x81",
				"\x02\x87\xC7",
				"\x06\x86\xC2\x81\xC3\x81\xC2",
				"\x02\x85\xCB",
				"\x06\x85\xC1\x81\xC7\x01\xC1",
				"\x08\x85\xC1\x81\xC1\x85\xC1\x01\xC1",
				"\x05\x88\xC2\x81\xC2\x81"
			}
		}, { // move left (1)
			{ // frame 0
				"\x05\x03\xC1\x85\xC1\x86",
				"\x0A\x01\xC1\x01\x81\xC1\x83\xC1\x82\xC1\x84",
				"\x07\x01\xC1\x01\xC7\x81\xC1\x84",
				"\x07\x01\xC3\x81\xC3\x81\xC3\x84",
				"\x03\x01\xCB\x84",
				"\x03\x02\xC9\x85",
				"\x05\x03\xC1\x85\xC1\x86",
				"\x05\x02\xC1\x87\xC1\x85"
			}, { // frame 1
				"\x05\x03\xC1\x85\xC1\x86",
				"\x06\x03\x81\xC1\x83\xC1\x87",
				"\x03\x03\xC7\x86",
				"\x07\x02\xC2\x81\xC3\x81\xC2\x85",
				"\x03\x01\xCB\x84",
				"\x07\x01\xC1\x01\xC7\x81\xC1\x84",
				"\x09\x01\xC1\x01\xC1\x85\xC1\x81\xC1\x84",
				"\x06\x03\x81\xC2\x81\xC2\x87"
			}
		}
	}, { // type 2
		{ // move right (0)
			{ // frame 0
				"\x03\x88\xC4\x82",
				"\x02\x85\xCA",
				"\x02\x84\xCC",
				"\x06\x84\xC3\x82\xC2\x82\xC3",
				"\x02\x84\xCC",
				"\x05\x87\xC2\x82\xC2\x81",
				"\x06\x86\xC2\x81\xC2\x81\xC2",
				"\x04\x84\xC2\x88\xC2"
			}, { // frame 1
				"\x03\x88\xC4\x82",
				"\x02\x85\xCA",
				"\x02\x84\xCC",
				"\x06\x84\xC3\x82\xC2\x82\xC3",
				"\x02\x84\xCC",
				"\x04\x86\xC3\x82\xC3",
				"\x06\x85\xC2\x82\xC2\x82\xC2",
				"\x04\x86\xC2\x84\xC2"
			}
		}, { // move left (1)
			{ // frame 0
				"\x04\x02\x82\xC4\x88",
				"\x03\x01\xCA\x85",
				"\x02\xCC\x84",
				"\x06\xC3\x82\xC2\x82\xC3\x84",
				"\x02\xCC\x84",
				"\x06\x02\x81\xC2\x82\xC2\x87",
				"\x07\x02\xC2\x81\xC2\x81\xC2\x86",
				"\x04\xC2\x88\xC2\x84"
			}, { // frame 1
				"\x04\x02\x82\xC4\x88",
				"\x03\x01\xCA\x85",
				"\x02\xCC\x84",
				"\x06\xC3\x82\xC2\x82\xC3\x84",
				"\x02\xCC\x84",
				"\x05\x02\xC3\x82\xC3\x86",
				"\x07\x01\xC2\x82\xC2\x82\xC2\x85",
				"\x05\x02\xC2\x84\xC2\x86"
			}
		}
	}
};

// [state][scanline]
uint8_t *gfx_player[][8] = {
	{ // right
		"\x03\x8A\xC1\x85",
		"\x03\x89\xC3\x84",
		"\x03\x89\xC3\x84",
		"\x02\x85\xCB",
		"\x02\x84\xCD",
		"\x02\x84\xCD",
		"\x02\x84\xCD",
		"\x02\x84\xCD"
	}, { // left
		"\x04\x01\x85\xC1\x8A",
		"\x04\x01\x84\xC3\x89",
		"\x04\x01\x84\xC3\x89",
		"\x03\x01\xCB\x85",
		"\x02\xCD\x84",
		"\x02\xCD\x84",
		"\x02\xCD\x84",
		"\x02\xCD\x84"
	}
};

// [state][scanline]
uint8_t *gfx_mothership[][7] = {
	{ // right (0)
		"\x03\x89\xC6\x84",
		"\x03\x87\xCA\x82",
		"\x03\x86\xCC\x81",
		"\x0A\x85\xC2\x81\xC2\x81\xC2\x81\xC2\x81\xC2",
		"\x02\x84\xD0",
		"\x07\x86\xC3\x82\xC2\x82\xC3\x81",
		"\x05\x87\xC1\x88\xC1\x82"
	}, { // left (1)
		"\x04\x01\x84\xC6\x89",
		"\x04\x01\x82\xCA\x87",
		"\x04\x01\x81\xCC\x86",
		"\x0B\x01\xC2\x81\xC2\x81\xC2\x81\xC2\x81\xC2\x85",
		"\x02\xD0\x84",
		"\x08\x01\x81\xC3\x82\xC2\x82\xC3\x86",
		"\x06\x01\x82\xC1\x88\xC1\x87"
	}
};

uint8_t *gfx_mothership_explode[] = {
	"\x0D\x82\xC1\x82\xC1\x81\xC1\x86\xC1\x81\xC1\x82\xC1\x81",
	"\x07\x83\xC1\x88\xC2\x84\xC1\x82",
	"\x08\xC1\x81\xC1\x83\xC4\x83\xC2\x86",
	"\x07\x85\xC7\x82\xC3\x82\xC1\x81",
	"\x0C\x84\xC3\x81\xC1\x81\xC1\x81\xC1\x82\xC3\x82\xC1",
	"\x07\x82\xC1\x83\xC5\x83\xC2\x85",
	"\x0A\xC1\x86\xC1\x81\xC1\x83\xC2\x83\xC1\x82",
	"\x09\x82\xC1\x83\xC1\x83\xC1\x84\xC1\x85"
};

uint8_t *gfx_missle_explode[] = {
	"\x05\xC1\x03\xC1\x02\xC1",
	"\x04\x02\xC1\x03\xC1",
	"\x02\x01\xC6",
	"\x01\xC8",
	"\x01\xC8",
	"\x02\x01\xC6",
	"\x04\x02\xC1\x02\xC1",
	"\x05\xC1\x02\xC1\x03\xC1"
};

uint8_t *gfx_missle_explode_eraser[] = { 
	"\x05\x81\x03\x81\x02\x81",
	"\x04\x02\x81\x03\x81",
	"\x02\x01\x86",
	"\x01\x88",
	"\x01\x88",
	"\x02\x01\x86",
	"\x04\x02\x81\x02\x81",
	"\x05\x81\x02\x81\x03\x81"
};

uint8_t *gfx_invader_explode[] = {
	"\x05\x84\xC1\x83\xC1\x84",
	"\x09\x81\xC1\x83\xC1\x81\xC1\x83\xC1\x81",
	"\x05\x82\xC1\x87\xC1\x82",
	"\x05\x83\xC1\x85\xC1\x83",
	"\x03\xC2\x89\xC2",
	"\x05\x83\xC1\x85\xC1\x83",
	"\x09\x82\xC1\x82\xC1\x81\xC1\x82\xC1\x82",
	"\x09\x81\xC1\x82\xC1\x83\xC1\x82\xC1\x81"
};

uint8_t *gfx_shield[] = {
	"\x02\x04\xCE",
	"\x02\x03\xD0",
	"\x02\x02\xD2",
	"\x02\x01\xD4",
	"\x01\xD6",
	"\x01\xD6",
	"\x01\xD6",
	"\x01\xD6",
	"\x01\xD6",
	"\x01\xD6",
	"\x01\xD6",
	"\x01\xD6",
	"\x03\xC7\x08\xC7",
	"\x03\xC6\x0A\xC6",
	"\x03\xC5\x0C\xC5",
	"\x03\xC5\x0C\xC5"
};

uint8_t *gfx_caron[] = {
	"\x03\xC1\x01\xC1",
	"\x02\x01\xC1"
};

// [frame][scanline]
uint8_t *gfx_player_explode[][1] = { 
	{ 
		""
	}, { 
		""
	}
};

uint8_t invader_type_by_row[][5] = { 
	{ 0, 1, 1, 2, 2 }, 
	{ 0, 0, 1, 1, 2 },
	{ 0, 0, 0, 1, 2 },
	{ 0, 0, 0, 0, 1 },
	{ 0, 0, 0, 0, 0 }
};

uint8_t invader_dx_by_type[] = { 2 << 1, 1 << 1, 0 };
uint8_t invader_width_by_type[] = { 8 << 1, 11 << 1, 12 << 1 };
uint8_t gfx_dx_by_state[] = { 4 << 1, 0 };

// config

const uint8_t cfg_missle_explode_frames = 12;          // x >= 1
const uint8_t cfg_invader_explode_frames = 12;         // x >= 1
const uint8_t cfg_shield_hit_frames = 12;			   // x >= 1
const uint8_t cfg_mothership_explode_frames = 36;      // x >= 1
const uint8_t cfg_missle_speed = 4;			           // x >= 4
const uint8_t cfg_invader_speed = 2 << 1;              // do not change!
const uint8_t cfg_mothership_wait_frames = 1;          // x >= 1
const uint8_t cfg_mothership_step = 1 << 1;       	   // 1 << 1 =< x <= 4 << 1  
const uint8_t cfg_mothership_spawn_delay_min_sec = 10; // x >= 0
const uint8_t cfg_mothership_spawn_delay_max_sec = 20; // x >= cfg_mothership_spawn_delay_min_sec
const uint8_t cfg_player_speed = 1 << 1;               // 1 << 1 =< x <= 4 << 1  

uint8_t cfg_invader_row_offset = 0;                    // x >= 0

uint8_t *to_binary_str(uint8_t val) { // WARNME: for debugging only
	for (uint8_t i = 0; i < 8; i++) {
		buffer[i] = (val & 128) != 0 ? '1' : '0';
		val <<= 1;
	}
	buffer[8] = 0;
	return buffer;
}

void shield_print_bits(shield *shield) { // WARNME: for debugging only
	for (uint8_t y = 0; y < 16; y++) {
		for (uint8_t i = 0; i < 3; i++) {
			uint8_t *str = to_binary_str(shield->bits[y][i]);
			avdc_write_str_at_cursor_pos(y, i * 8 + i, str, NULL);
		}
	}
}

void shield_erase_bits(shield *shield, int8_t x, int8_t y, uint8_t bits) { // local coords
	// WARNME: we assume that x is within bounds
	uint16_t *dest;
	if (y >= 0 && y <= 15) {
		if (x >= 8) {
			x -= 8;
			dest = &shield->bits[y][1];
		} else {
			dest = &shield->bits[y][0];
		}
		uint16_t bits_16 = bits;
		bits_16 = x <= 8 ? ~(bits_16 << (8 - x)) : ~(bits_16 >> (x - 8));
		*dest &= (bits_16 >> 8) | (bits_16 << 8); 
	}
}

void shield_handle_invader_collision(invader *inv, object_state state) {
	if (inv->y >= 197) {
		uint16_t inv_left = inv->x - gfx_dx_by_state[state]; // inclusive
		uint16_t inv_right = inv_left + (16 << 1); // exclusive
		for (uint8_t i = 0; i < 4; i++) {
			uint16_t shield_left = shields[i].x; // inclusive
			uint16_t shield_right = shield_left + (22 << 1); // exclusive
			if (inv_left < shield_right && inv_right > shield_left) { 
				// collision detected
				// clear 4 x 8 pixels
				if (state == STATE_MOVE_LEFT) {				
					gdp_fill_rect(GDP_TOOL_ERASER, inv_left, inv->y, 4 << 1, 8);
				} else {
					gdp_fill_rect(GDP_TOOL_ERASER, inv_right - (4 << 1), inv->y, 4 << 1, 8);
				}
				// clear 16 bits in 8 shield rows
				int8_t y_local = inv->y - 197;
				int8_t x_local = (inv_left - shields[i].x) >> 1; 
				for (uint8_t j = 0; j < 8; j++) {
					shield_erase_bits(&shields[i], x_local, y_local + j, 255);
					shield_erase_bits(&shields[i], x_local + 8, y_local + j, 255);
				}
			}
		}
	}
}

void grid_reset() {
	for (uint8_t i = 0; i < 11; i++) {
		grid.cols[i].x_min = grid.cols_new[i].x_min;
		grid.cols_new[i].x_min = 1024;
		grid.cols[i].x_max = grid.cols_new[i].x_max;
		grid.cols_new[i].x_max = 0;
	}
	for (uint8_t i = 0; i < 11; i++) {
		if (grid.cols[i].invader_count > 0) {
			grid.col_non_empty_first = i;
			break;
		}
	}
	for (int8_t i = 10; i >= 0; i--) {
		if (grid.cols[i].invader_count > 0) { 
			grid.col_non_empty_last = i;
			break;
		}
	}
}

void grid_update(uint8_t col_idx, uint16_t x_min, uint16_t x_max) {
	if (grid.cols[col_idx].x_min > x_min) { grid.cols[col_idx].x_min = x_min; }
	if (grid.cols[col_idx].x_max < x_max) { grid.cols[col_idx].x_max = x_max; }
	if (grid.cols_new[col_idx].x_min > x_min) { grid.cols_new[col_idx].x_min = x_min; }
	if (grid.cols_new[col_idx].x_max < x_max) { grid.cols_new[col_idx].x_max = x_max; }
}

void invader_draw(invader *inv, object_state state) {
	uint8_t **sprite = gfx_invaders[inv->type][state][inv->frame];
	gdp_draw_sprite(sprite, 7, inv->x - gfx_dx_by_state[state], inv->y);
}

void invader_move_right(invader *inv) {
	inv->x += cfg_invader_speed;
	inv->frame = ~inv->frame & 1;
	shield_handle_invader_collision(inv, STATE_MOVE_RIGHT);
	invader_draw(inv, STATE_MOVE_RIGHT);
}

void invader_move_left(invader *inv) {
	inv->x -= cfg_invader_speed;
	inv->frame = ~inv->frame & 1;
	shield_handle_invader_collision(inv, STATE_MOVE_LEFT);
	invader_draw(inv, STATE_MOVE_LEFT);
}

void invader_clear(invader *inv) {
	gdp_fill_rect(GDP_TOOL_ERASER, inv->x, inv->y, 12 << 1, 8);
}

bool invader_check_reach_bottom(invader *inv) {
	if (inv->y == 221) {
		invader_draw(inv, STATE_MOVE_LEFT);
		return true;
	}
	return false;
}

bool invader_move_down_left(invader *inv) {
	invader_clear(inv);
	inv->y += 8;
	inv->x -= cfg_invader_speed;
	inv->frame = ~inv->frame & 1;
	if (!invader_check_reach_bottom(inv)) {
		shield_handle_invader_collision(inv, STATE_MOVE_LEFT);
		invader_draw(inv, STATE_MOVE_LEFT);
		return false;
	}
	return true;
}

bool invader_move_down_right(invader *inv) {
	invader_clear(inv);
	inv->y += 8;
	inv->x += cfg_invader_speed;
	inv->frame = ~inv->frame & 1;	
	if (!invader_check_reach_bottom(inv)) {
		shield_handle_invader_collision(inv, STATE_MOVE_RIGHT);
		invader_draw(inv, STATE_MOVE_RIGHT);
		return false;
	}
	return true;
}

bool invader_check_hit(invader *inv, uint16_t x, uint8_t y_top, uint8_t y_bottom) {
	uint16_t inv_x = inv->x + inv->dx;
	return x >= inv_x && x <= inv_x + inv->width - 1 
		&& ((y_top <= inv->y && y_bottom >= inv->y) || (y_top <= inv->y + 7 && y_bottom >= inv->y + 7));
}

void invader_explode_draw(invader *inv) {
	gdp_draw_sprite(gfx_invader_explode, 7, inv->x, inv->y);
}

void invader_explode_clear(invader *inv) {
	gdp_fill_rect(GDP_TOOL_ERASER, inv->x, inv->y, 13 << 1, 8);	
}

void player_draw(object_state state) {
	uint8_t **sprite = gfx_player[state];
	gdp_draw_sprite(sprite, 7, player.x - gfx_dx_by_state[state], 221);
}

void player_move_right() {
	player.x += cfg_player_speed;
	player_draw(STATE_MOVE_RIGHT);
}

void player_move_left() {
	player.x -= cfg_player_speed;
	player_draw(STATE_MOVE_LEFT);
}

void mothership_draw(object_state state) {
	gdp_draw_sprite(gfx_mothership[state], 6, mothership.x - gfx_dx_by_state[state], 38);
}

void mothership_move_right() {
	mothership.x += cfg_mothership_step;
	mothership_draw(STATE_MOVE_RIGHT);
}

void mothership_move_left() {
	mothership.x -= cfg_mothership_step;
	mothership_draw(STATE_MOVE_LEFT);
}

void mothership_clear() {
	gdp_fill_rect(GDP_TOOL_ERASER, mothership.x, 38, 16 << 1, 8);	
}

bool mothership_check_hit(uint16_t x, uint8_t y_top, uint8_t y_bottom) {
	return x >= mothership.x && x <= mothership.x + (16 << 1) - 1 
		&& ((y_top <= 38 && y_bottom >= 38) || (y_top <= 38 + 6 && y_bottom >= 38 + 6));
}

void mothership_explode_draw() {
	gdp_draw_sprite(gfx_mothership_explode, 7, mothership.x, 37);	
}

void mothership_explode_clear() {
	gdp_fill_rect(GDP_TOOL_ERASER, mothership.x, 37, 21 << 1, 8);
}

void mothership_timer_reset() {
	timer_reset(0);
	mothership.spawn_delay_sec = cfg_mothership_spawn_delay_min_sec + 
		(rand() % (cfg_mothership_spawn_delay_max_sec - cfg_mothership_spawn_delay_min_sec + 1));
}

void missle_explode_draw() {
	gdp_draw_sprite(gfx_missle_explode, 7, missle.x - 6, 30);
}

void missle_explode_clear() {
	gdp_fill_rect(GDP_TOOL_ERASER, missle.x - 6, 30, 8 << 1, 8);	
}

void shield_draw(shield *shield, uint8_t start_row) {
	gdp_draw_sprite(&gfx_shield[start_row], 15 - start_row, shield->x, 197 + start_row);
}

void shield_make_damage_player(shield *shield, uint16_t x, uint8_t y) { // world coords
	int8_t y_local = y - 197 - 3;
	int8_t x_local = ((x - shield->x) >> 1) - 3; 
	// update bits (shield image)
	for (uint8_t i = 0; i < 8; i++) {
		shield_erase_bits(shield, x_local, y_local + i, bits_shield_damage_player[i]);
	}
	// render damage
	gdp_draw_sprite(gfx_missle_explode_eraser, 7, x - (3 << 1), y - 3);
}

bool shield_check_hit_pixel(shield *shield, uint8_t x_local, int8_t y_local) { // local coords
	// WARNME: we assume that x_local is within bounds
	if (y_local < 0 || y_local > 15) { return false; }
	uint8_t byte = shield->bits[y_local][x_local >> 3];
	return ((byte << (x_local & 7)) & 128) != 0;
}

// WARNME: this checks from bottom to top (player shooting)
uint8_t shield_check_hit(shield *shield, uint16_t x, uint8_t y_top, uint8_t y_bottom) { // returns world coords of hit, or 0
	// check bounding box
	if (x >= shield->x && x < shield->x + (22 << 1) 
	 	&& ((y_top <= 197 && y_bottom >= 197) || (y_top <= 197 + 16 && y_bottom >= 197 + 16) || (y_top >= 197 && y_bottom <= 197 + 16))) { // NOTE: we need the 3rd condition here because the shield is higher than 8 pixels
		// check pixels
		uint8_t x_local = (x - shield->x) >> 1; 
		for (uint8_t y = y_bottom; y >= y_top; y--) {
			int8_t y_local = y - 197;
			if (shield_check_hit_pixel(shield, x_local, y_local)) {
				return y;
			}
		}
	}
	return 0;
}

// main game loop

void _render_background(uint8_t *file_name) {
	int fd = open(file_name, O_RDONLY);
	ssize_t read_size = 0;
	size_t tail = 0;
	uint16_t y = 0;

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
	    tail = eod - row;
	    if (tail > 0) { // WARNME: do I need this?
	    	memcpy(buffer, row, tail);
		}
	} while (y <= 255);
	close(fd);

	gdp_set_display_page(gdp_write_page);
}

bool missle_handle_hit() {
	for (uint8_t col = 0; col < 11; col++) {
		if (missle.x >= grid.cols[col].x_min && missle.x <= grid.cols[col].x_max) {
			// we might have hit an invader in column 'col'
			for (int8_t row = 4; row >= 0; row--) { 
				invader *inv = &invaders[row * 11 + col];
				if (!inv->destroyed && invader_check_hit(inv, missle.x, missle.y - 3, missle.y + cfg_missle_speed)) { 
					inv->destroyed = true;
					grid.cols[inv->col].invader_count--;
					missle.explode_frame = cfg_invader_explode_frames; 
					missle.hit_invader = inv;
					invader_explode_draw(inv);
					// remove from linked list 
					if (inv->prev != NULL) {
						inv->prev->next = inv->next;
					} else {
						first_inv = inv->next;
					}
					if (inv->next != NULL) {
						inv->next->prev = inv->prev;
					}
					return true;
				}
			}
		}
	}
	if (mothership_check_hit(missle.x, missle.y - 3, missle.y + cfg_missle_speed)) { 
		missle.hit_mothership = true;
		missle.explode_frame = cfg_mothership_explode_frames;
		mothership_explode_draw(); 
		return true;
	} else {
		// TODO: check missle y first?
		for (uint8_t i = 0; i < 4; i++) {
			if (missle.hit_shield_y = shield_check_hit(&shields[i], missle.x, missle.y - 3, missle.y + cfg_missle_speed)) {
				missle.hit_shield = &shields[i];
				missle.explode_frame = cfg_shield_hit_frames;
				gdp_draw_sprite(gfx_missle_explode, 7, missle.x - (3 << 1), missle.hit_shield_y - 3);
				return true;
			}
		}
	}
	return false;
}

user_action game_intro_keyboard_handler(uint16_t timeout_ms) {
	timer_reset(0);
	uint16_t timeout = timeout_ms / 10;
	do {
		char key;
		if (key = kbhit()) {
			switch (key) {
				case 'x':
				case 'X':
					return ACTION_EXIT;
				default:
					return ACTION_CONTINUE;
			}
		}
	} while (timer_diff() < timeout);
	return ACTION_NONE;
}

user_action render_plain_text(uint8_t *text, int delay) {
    uint8_t i = 0;
    user_action action;
    while (text[i] != 0) {
    	if (delay > 0) {
	   		if (action = game_intro_keyboard_handler(delay)) { return action; }
	   	}
        gdp_wait_ready();
    	GDP_CTRL_1 = GDP_CTRL_1_TOOL_DOWN | GDP_TOOL_PEN;
    	uint8_t ch = text[i++];
        GDP_CMD = ch;
        gdp_wait_ready();
	    GDP_CTRL_1 = GDP_CTRL_1_TOOL_UP; 
	    GDP_DX = 2 << 1;
	    GDP_CMD = GDP_CMD_LINE_DX_POS;
    }
    return ACTION_NONE;
}

user_action render_text_at(uint16_t x, uint8_t y, uint8_t *text, int delay) {
	uint8_t *p_e = buffer, *p_s = buffer;
	uint16_t x_e = x;
	user_action action;
	gdp_set_xy(x, y);
	//strcpy(buffer, text); // WARNME: this doesn't work correctly
	// strcpy
	uint8_t i = 0;
	while (text[i] != 0) {
		buffer[i] = text[i];
		i++;
	}
	buffer[i] = 0;
	// end of strcpy
	while (*p_e != 0) {
		if (*p_e == '^') {
			// write text from p_s to p_e
			*p_e = 0;
			if (action = render_plain_text(p_s, delay)) { return action; }
			x_e += (strlen(p_s) * 8) << 1;
			p_s = p_e + 1;
			// draw caron
			gdp_draw_sprite(gfx_caron, 1, x_e - (7 << 1), y - 9);
			gdp_set_xy(x_e, y);		
		}
		p_e++;
	}
	return render_plain_text(p_s, delay);
}

void render_score() {
	gdp_fill_rect(GDP_TOOL_ERASER, 161 << 1, 21, 29 << 1, 7);
	itoa((long)score + 10000, buffer, 10);
	render_text_at(161 << 1, 21 + 7, buffer + 1, 0);
}

void render_level() {
	gdp_fill_rect(GDP_TOOL_ERASER, 254 << 1, 21, 5 << 1, 7);
	render_text_at(254 << 1, 21 + 7, itoa(level, buffer, 10), 0);
}

void render_hi_score() {
	gdp_fill_rect(GDP_TOOL_ERASER, 321 << 1, 21, 29 << 1, 7);
	itoa((long)hi_score + 10000, buffer, 10);
	render_text_at(321 << 1, 21 + 7, buffer + 1, 0);
}

void render_credits() {
	gdp_fill_rect(GDP_TOOL_ERASER, 345 << 1, 244, 13 << 1, 7);
	itoa((long)credits + 100, buffer, 10);
	render_text_at(345 << 1, 244 + 7, buffer + 1, 0);
}

void render_lives() {
	if (lives <= 2) {
		gdp_fill_rect(GDP_TOOL_ERASER, 187 << 1, 243, 13 << 1, 8);
		if (lives <= 1) {
			gdp_fill_rect(GDP_TOOL_ERASER, 170 << 1, 243, 13 << 1, 8);
		}
	}
	if (lives >= 2) {
		gdp_draw_sprite(gfx_player[STATE_MOVE_LEFT], 7, 170 << 1, 243);
		if (lives >= 3) {
			gdp_draw_sprite(gfx_player[STATE_MOVE_LEFT], 7, 187 << 1, 243);
		}
	}
	gdp_fill_rect(GDP_TOOL_ERASER, 153 << 1, 244, 5 << 1, 7);
	render_text_at(153 << 1, 244 + 7, itoa(lives, buffer, 10), 0);
}

void render_clear_line(uint8_t y) {
	gdp_set_xy(256, y);
	gdp_line_dx_pos(GDP_TOOL_ERASER, GDP_STYLE_NORMAL, 255);
	gdp_set_xy(512, y);
	gdp_line_dx_pos(GDP_TOOL_ERASER, GDP_STYLE_NORMAL, 255);
}

void render_cls() {
	for (uint8_t y = 30; y < 242; y += 2) {
		render_clear_line(y);
	}
	for (uint8_t y = 31; y < 242; y += 2) {
		render_clear_line(y);
	}
}

void render_game_over() {
	msleep(5000);
	// TODO
}

user_action game_intro() {
	user_action action;
	if (/*(action = game_intro_keyboard_handler(1000)) ||*/
		(action = render_text_at(238 << 1, 63 + 7, "IGRAJ", 100)) ||
		(action = render_text_at(194 << 1, 87 + 7, "NAPAD IZ VESOLJA", 100))) { 
		return action; 
	}
	render_text_at(162 << 1, 119 + 7, "*TOC^KOVALNA PREGLEDNICA*", 0);
	gdp_draw_sprite(gfx_mothership[STATE_MOVE_LEFT], 6, 217 << 1, 135);
	gdp_draw_sprite(gfx_invaders[0][STATE_MOVE_LEFT][0], 7, 219 << 1, 150);
	gdp_draw_sprite(gfx_invaders[1][STATE_MOVE_LEFT][1], 7, 219 << 1, 166);
	gdp_draw_sprite(gfx_invaders[2][STATE_MOVE_LEFT][0], 7, 219 << 1, 182);
	if ((action = render_text_at(234 << 1, 135 + 7, "=NEZNANO", 100)) ||
		(action = render_text_at(234 << 1, 151 + 7, "=30 TOC^K", 100)) ||
		(action = render_text_at(234 << 1, 167 + 7, "=20 TOC^K", 100)) ||
		(action = render_text_at(234 << 1, 183 + 7, "=10 TOC^K", 100))) { 
		return action; 
	}
	while (true) {
		char key;
		if (key = kbhit()) {
			switch (key) {
				case 'x':
				case 'X':
					return ACTION_EXIT;
				case 'w':
				case 'W':
					if (level < 9) {
						level++;
						render_level();
					}
					break;
				case 's':
				case 'S':
					if (level > 1) {
						level--;
						render_level();
					}
					break;
				case 'k':
				case 'K':
					if (credits < 10) {
						credits++;
						render_credits();
					}
					break;
				default:
					return ACTION_CONTINUE;
			}
		}
	}
}

void game_init() {
	// init level
	cfg_invader_row_offset = level - 1;

	// init grid 
	for (uint8_t i = 0; i < 11; i++) {
		grid.cols[i].x_min = grid.cols_new[i].x_min = 1024;
		grid.cols[i].x_max = grid.cols_new[i].x_max = 0;
		grid.cols[i].invader_count = 5;
	}
	grid.col_non_empty_last = 10;
	grid.col_non_empty_first = 0;

	// init invaders
	uint8_t i = 0;
	uint8_t j = cfg_invader_row_offset << 3; 
	for (uint8_t y = 61 + j; y <= 125 + j; y += 16) {
		for (uint16_t x = 169; x <= 329; x += 16) {
			invader *inv = &invaders[i];
			inv->type = invader_type_by_row[(level - 1) / 2][i / 11];
			inv->width = invader_width_by_type[inv->type];
			inv->dx = invader_dx_by_type[inv->type];
		 	inv->frame = 0;
		 	inv->x = x << 1;
		 	inv->y = y;
		 	inv->state = STATE_MOVE_RIGHT;
		 	inv->destroyed = false;
		 	// This bit is complicated. Don't feel like refactoring. Life is too short.
		 	inv->col = i % 11;
			inv->next = (i == 10) ? NULL : &invaders[ (i + 1) % 11 == 0 ? i - 21 : i + 1 ];
			inv->prev = (i == 44) ? NULL : &invaders[ i % 11 == 0 ? i + 21 : i - 1 ];
		 	grid_update(inv->col, inv->x, inv->x + (12 << 1) - 1); 
		 	i++;
		}
	}

	first_inv = &invaders[44];

	// init player 
	player.x = 161 << 1;
	player.state = STATE_HOLD;

	// init missle
	missle.x = 0; // not fired
	missle.explode_frame = 0; // not exploding	
	missle.hit_invader = NULL;
	missle.hit_shield = NULL;
	missle.hit_shield_y = 0;
	missle.hit_mothership = false;

	// init mothership
	mothership.x = 0;
	mothership.wait_frame = 0;

	mothership_timer_reset();

	// init shields
	for (uint8_t i = 0; i < 4; i++) {
		shields[i].x = (177 + i * 45) << 1;
		for (uint8_t j = 0; j < 8; j++) {
			for (uint8_t k = 0; k < 3; k++) {
				shields[i].bits[j][k] = level >= 5 ? 0 : shield_bits[j][k];
			}
		}		
		for (uint8_t j = 8; j < 16; j++) {
			for (uint8_t k = 0; k < 3; k++) {
				shields[i].bits[j][k] = level >= 9 ? 0 : shield_bits[j][k];
			}
		}
	}

	// render stuff
	render_cls();
	render_lives();
	for (uint8_t i = 0; i < 55; i++) {
		invader_draw(&invaders[i], STATE_MOVE_RIGHT);
	}
	player_draw(STATE_MOVE_RIGHT);
	if (level < 9) {
		for (uint8_t i = 0; i < 4; i++) {
			shield_draw(&shields[i], level >= 5 ? 8 : 0);
		}
	}
}

game_state game() {
	while (true) {
		invader *inv = first_inv;
		if (inv == NULL) {
			// we destroyed all of them
			return GAME_STATE_LEVEL_CLEARED;
		}
		do {
			// update next invader
			if (inv->state == STATE_MOVE_RIGHT) {
				if (inv->x < (347 - (grid.col_non_empty_last - inv->col) * 16) << 1) {
					invader_move_right(inv);
				} else {
					if (invader_move_down_left(inv)) {
						render_game_over();
						return GAME_STATE_GAME_OVER;
					}
					inv->state = STATE_MOVE_LEFT;
				}
			} else {
				if (inv->x > (151 + (inv->col - grid.col_non_empty_first) * 16) << 1) {
					invader_move_left(inv);
				} else {
					if (invader_move_down_right(inv)) {
						render_game_over();
						return GAME_STATE_GAME_OVER;
					}
					inv->state = STATE_MOVE_RIGHT;
				}
			}
			// update grid
			grid_update(inv->col, inv->x, inv->x + (12 << 1) - 1); 
			if (inv->next == NULL) {
				grid_reset();
			}
			// get keyboard input
			char key;
			if (key = kbhit()) { 
				switch (key) {
				case 'a':
				case 'A':
					// left / stop
					player.state = player.state == STATE_MOVE_LEFT ? STATE_HOLD : STATE_MOVE_LEFT;
					break;
				case 'd':
				case 'D':
					// right / stop
					player.state = player.state == STATE_MOVE_RIGHT ? STATE_HOLD : STATE_MOVE_RIGHT;
					break;
				case ' ':
					// fire
					if (missle.x == 0) {
						missle.x = player.x + (6 << 1);
						missle.y = 220 + cfg_missle_speed;
					}
					break;
				case 'b':
				case 'B':
					shield_print_bits(&shields[0]);
					break;
				case 'x':
				case 'X':
					return GAME_STATE_EXIT;
				case 'g':
				case 'G':
					return GAME_STATE_GAME_OVER;
				case 'n':
				case 'N':
					return GAME_STATE_LEVEL_CLEARED;
				default:
					// stop
					player.state = STATE_HOLD;
					break;
				}
			}
			// update missle
			if (missle.explode_frame > 0) {
				if (missle.explode_frame == 1) {
					if (missle.hit_mothership) {
						mothership_explode_clear();
						missle.hit_mothership = false;
						mothership.x = 0;
					} else if (missle.hit_shield_y) {
						shield_make_damage_player(missle.hit_shield, missle.x, missle.hit_shield_y); 
						missle.hit_shield_y = 0;
					} else if (missle.hit_invader) {
						invader_explode_clear(missle.hit_invader);
						missle.hit_invader = NULL;
					} else {
						missle_explode_clear();
					}
					missle.x = 0;
				}
				missle.explode_frame--;
			} 
			else if (missle.x > 0) {
				if (missle.y < 220 + cfg_missle_speed) {
					gdp_set_xy(missle.x, missle.y);
					gdp_line_delta(GDP_TOOL_ERASER, GDP_STYLE_NORMAL, /*dx*/0, /*dy*/3, GDP_DELTA_SIGN_DY_NEG);
					gdp_set_xy(missle.x + 1, missle.y);
					gdp_line_delta(GDP_TOOL_ERASER, GDP_STYLE_NORMAL, /*dx*/0, /*dy*/3, GDP_DELTA_SIGN_DY_NEG);
				}
				missle.y -= cfg_missle_speed;
				if (missle.y <= 37) {
					missle_explode_draw();
					missle.explode_frame = cfg_missle_explode_frames;
				} else {
					if (!missle_handle_hit()) {
						gdp_set_xy(missle.x, missle.y);
						gdp_line_delta(GDP_TOOL_PEN, GDP_STYLE_NORMAL, /*dx*/0, /*dy*/3, GDP_DELTA_SIGN_DY_NEG);
						gdp_set_xy(missle.x + 1, missle.y);
						gdp_line_delta(GDP_TOOL_PEN, GDP_STYLE_NORMAL, /*dx*/0, /*dy*/3, GDP_DELTA_SIGN_DY_NEG);
					}
				}
			}
			// update player
			switch (player.state) {
				case STATE_MOVE_LEFT:
					if (player.x > (161 << 1)) {
						player_move_left();
					}
					break;
				case STATE_MOVE_RIGHT:
					if (player.x < (337 << 1)) {
						player_move_right();
					}
					break;
				default:
					msleep(5); // compensate for player not moving
					break;
			}
			// update mothership
			if (!missle.hit_mothership) {
				if (mothership.x == 0) {
					if (timer_diff() / 100 >= mothership.spawn_delay_sec) {
						mothership.state = rand() % 2 == 0 
							? STATE_MOVE_LEFT 
							: STATE_MOVE_RIGHT;
						mothership.x = mothership.state == STATE_MOVE_LEFT 
							? 334 << 1 
							: 161 << 1;
						mothership_draw(mothership.state); 
						mothership_timer_reset();
					} else {
						msleep(3); // compensate for mothership not rendering
					}
				} else {
					if (mothership.wait_frame == cfg_mothership_wait_frames) {
						mothership.wait_frame = 0;
						bool able_to_move = false;
						if (mothership.state == STATE_MOVE_LEFT) {
							if ((mothership.x > 161 << 1) && (able_to_move = true)) { // single '=' is OK here :-)
								mothership_move_left();
							}
						} else {
							if ((mothership.x < 334 << 1) && (able_to_move = true)) {
								mothership_move_right();
							} 
						}
						if (!able_to_move) {
							mothership_clear();
							mothership.x = 0;
						}
					} else {
						mothership.wait_frame++;
					}
				}
			}
		} while (inv = inv->next);
	}
}

int main() {
	gdp_init();
	avdc_init();

	srand(timer());

	while (true) {
		score = 0;
		lives = 3;
		credits = credits > 1 ? credits : 1;
		level = 1;
		gdp_cls();
		//_render_background("bkg2.bin");
		render_text_at(153 << 1, 5 + 7, "TOC^KE", 0);
		render_text_at(230 << 1, 5 + 7, "STOPNJA", 0);
		render_text_at(313 << 1, 5 + 7, "REKORD", 0);
		render_text_at(282 << 1, 244 + 7, "KOVANCI", 0);
		render_score();	
		render_level();
		render_hi_score();
		render_credits();
		if (game_intro() == ACTION_EXIT) { break; }
		credits--;
		render_credits();
		while (true) {
			game_init(); 
			game_state state = game(); 
			if (state == GAME_STATE_EXIT) { 
				break; 
			} else if (state == GAME_STATE_LEVEL_CLEARED) {
				level++;
				if (level == 10) { level = 9; }
				render_level();
			} else { //if (state == GAME_STATE_GAME_OVER) {
				if (credits == 0) { 
					break; 
				}
				lives = 3;
				credits--;
				//render_lives(); // NOTE: done by game_init
				render_credits();
			}
		}
	}

	avdc_done();
	gdp_done();

	return 0;
}