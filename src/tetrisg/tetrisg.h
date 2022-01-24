#ifndef __TETRISG_H__
#define __TETRISG_H__

#include <stdint.h>
#include <stdbool.h>
#include "../common/gdp.h"

#define BUFFER_SIZE 3072

// types 

typedef uint8_t *p_uint8_t;

// state

typedef enum {
	STATE_PLAY,
	STATE_PAUSE,
	STATE_GAME_OVER
} state;

extern state game_state;
extern bool show_next;
extern bool show_text;
extern uint8_t level;
extern uint8_t steps;
extern long score;
extern uint8_t full_lines;
extern long time_passed;

void state_init();

// key

typedef enum {
	KEY_NONE,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_ROTATE,
	KEY_DROP,
	KEY_PAUSE,
	KEY_SHOW_NEXT,
	KEY_RESTART,
	KEY_EXIT,
	KEY_OTHER
} key;

key key_get();

// playfield

extern uint8_t playfield[19][10];

void playfield_init();
void playfield_update_bkgr();
void playfield_clear_row(uint8_t row);
void playfield_move_row(uint8_t row_src, uint8_t row_dst);
uint8_t playfield_collapse();

// gfx

extern uint8_t *gfx_popup_loader[];
extern uint8_t *gfx_playfield_row_empty[];
extern uint8_t *gfx_blocks[][7];
// TODO: add other gfx
extern uint8_t *gfx_sprite_logo[];

// render

void render_init();
void render_pause();
void render_clear_pause();
void render_next_block();
void render_clear_next_block();
void render_level();
void render_score();
void render_full_lines();
void render_game_over();
void render_clear_block();
void render_block(int8_t dx, int8_t dy, uint8_t rot_prev);
void render_block_fallen();
void render_playfield_row(uint8_t row);

// block

extern p_uint8_t blocks[][4][4];

extern int8_t block_type;
extern uint8_t block_rot;
extern int8_t block_pos_x;
extern int8_t block_pos_y;

extern int8_t block_type_next;

void block_init();
bool block_move_left();
bool block_move_right();
bool block_move_down();
bool block_rotate();
bool block_drop();
bool block_check(int8_t pos_x, int8_t pos_y, uint8_t rot);
bool block_next();

// timer

bool timer_done();

// game

void game_init();
bool game_play();

// utils

void _render_popup_blank();
void _render_popup_area(uint8_t **gfx);
void _render_loader();
void _render_loader_bar(uint16_t val, uint16_t max);
void _render_block_piece_fast(uint16_t x, uint8_t y, gdp_tool tool);
void _render_block_piece(uint16_t x, uint8_t y, uint8_t block_type);
void _render_playfield_cell(uint16_t x, uint8_t y, uint8_t block_type);
void _render_background(uint8_t *file_name, bool render_playfield);
void _render_background_title();
void _render_message(uint8_t *text_1, uint8_t *text_2);
void _render_title();

uint8_t _block_get_piece(uint8_t rot, int8_t r, int8_t c);

void _gfx_load(uint8_t *file_name, uint8_t height);

#endif