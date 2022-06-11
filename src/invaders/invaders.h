#ifndef __INVADERS_H__
#define __INVADERS_H__

#include <stdint.h>
#include <stdbool.h>

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
	STATE_MOVE_LEFT = 1,
	STATE_HOLD = 2
} object_state;

typedef enum {
	GAME_STATE_EXIT,
	GAME_STATE_LEVEL_CLEARED,
	GAME_STATE_GAME_OVER
} game_state;

typedef enum {
	ACTION_NONE = 0,
	ACTION_EXIT = 1,
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

typedef struct {
	bool active;
	uint8_t type;
	uint8_t frame;
	uint16_t x;
	uint8_t y;
	uint8_t explode_frame;
} bullet;

// debugging (TODO: remove this)

uint8_t *to_binary_str(uint8_t val);
void shield_print_bits(shield *shield);

// shield

void shield_erase_bits(shield *shield, int8_t x, int8_t y, uint8_t bits);
void shield_handle_invader_collision(invader *inv, object_state state);

// grid

void grid_reset();
void grid_update(uint8_t col_idx, uint16_t x_min, uint16_t x_max);

// invader

void invader_draw(invader *inv, object_state state);
void invader_move_right(invader *inv);
void invader_move_left(invader *inv);
void invader_clear(invader *inv);
bool invader_check_reach_bottom(invader *inv);
bool invader_move_down_left(invader *inv);
bool invader_move_down_right(invader *inv);
bool invader_check_hit(invader *inv, uint16_t x, uint8_t y_top, uint8_t y_bottom);
void invader_explode_draw(invader *inv);
void invader_explode_clear(invader *inv);

// player

void player_draw(object_state state);
void player_move_right();
void player_move_left();
void player_score_update(uint16_t points);
void player_explode_animate();
bool player_check_hit(uint16_t x, uint8_t y_top, uint8_t y_bottom);

// mothership

void mothership_draw(object_state state);
void mothership_move_right();
void mothership_move_left();
void mothership_clear();
bool mothership_check_hit(uint16_t x, uint8_t y_top, uint8_t y_bottom);
void mothership_explode_draw();
void mothership_explode_clear();
void mothership_score_draw();
void mothership_timer_reset();

// missle

void missle_explode_draw();
void missle_explode_clear();
bool missle_handle_hit();

// bullet

void bullet_create();
void bullet_draw(bullet *b);
void bullet_explode_draw(bullet *b);
void bullet_explode_clear(bullet *b);
void bullet_clear_trail(bullet *b);
void bullet_clear_leftover(bullet *b);

// shield

void shield_draw(shield *shield, uint8_t start_row);
void shield_make_damage(shield *shield, uint16_t x, uint8_t y, uint8_t *bits);
bool shield_check_hit_pixel(shield *shield, uint8_t x_local, int8_t y_local);
uint8_t shield_check_hit(shield *shield, uint16_t x, uint8_t y_top, uint8_t y_bottom, bool from_bottom);

// render

user_action render_plain_text(uint8_t *text, int delay);
user_action render_text_at(uint16_t x, uint8_t y, uint8_t *text, int delay);
void render_score();
void render_level();
void render_hi_score();
void render_credits();
void render_lives();
void render_clear_line(uint8_t y);
void render_cls();

// game

user_action game_intro_keyboard_handler(uint16_t timeout_ms);
user_action game_intro();
void game_init();
void game_over();
game_state game();

// main

int main();

#endif