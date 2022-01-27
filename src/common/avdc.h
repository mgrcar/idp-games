#ifndef __AVDC_H__
#define __AVDC_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	AVDC_MODE_80,
	AVDC_MODE_132,
	AVDC_MODE_CUSTOM
} avdc_mode;

#define AVDC_DEFAULT_ATTR 0x00
#define AVDC_ATTR_UDG     0x04

#define AVDC_STATUS_READY 0x20
#define AVDC_ACCESS_FLAG  0x10

#define AVDC_CMD_CUR_OFF      0x30
#define AVDC_CMD_CUR_ON       0x31
#define AVDC_CMD_READ_AT_PTR  0xA4
#define AVDC_CMD_SET_PTR_REG  0x1A
#define AVDC_CMD_SET_MODE_REG 0x15
#define AVDC_CMD_WRITE_AT_PTR 0xA2
#define AVDC_CMD_WRITE_AT_CUR 0xAB
#define AVDC_CMD_WRITE_C2P    0xBB
#define AVDC_CMD_RESET        0x00
#define AVDC_CMD_SET_REG_0    0x10
#define AVDC_CMD_DISPLAY_ON   0x3D

#define EI __asm__ ("NOP")
#define DI __asm__ ("NOP") // WARNME: test if DI f* up the keyboard

__sfr __at 0x39 AVDC_CMD;     // W: command 
__sfr __at 0x39 AVDC_STATUS;  // R: status (ready)
__sfr __at 0x36 AVDC_ACCESS;  // R: status (access)
__sfr __at 0x38 AVDC_INIT;    // W: write to current IR (init/interrupt register) 
__sfr __at 0x34 AVDC_CHR;     // R/W: character register 
__sfr __at 0x35 AVDC_ATTR;    // R/W: attribute register 

__sfr __at 0x3C AVDC_CUR_LWR; // W: cursor address lower
__sfr __at 0x3D AVDC_CUR_UPR; // W: cursor address upper

__sfr __at 0x32 AVDC_COMMON_TXT_ATTR;  
__sfr __at 0x3E AVDC_SCREEN_START_2_LOWER;
__sfr __at 0x3F AVDC_SCREEN_START_2_UPPER; 
__sfr __at 0x20 AVDC_GDP_STATUS;

extern uint16_t row_addr[26];

void avdc_init();
void avdc_init_ex(avdc_mode mode, uint8_t custom_txt_attr_reg, uint8_t *custom_init_str);
void avdc_reset(avdc_mode mode, uint8_t custom_txt_attr_reg, uint8_t *custom_init_str);

void avdc_wait_access(); // WARNME: disables interrupts (you need to call EI to re-enable them)
void avdc_wait_ready();

void avdc_cursor_off();
void avdc_cursor_on();

void avdc_clear_screen();
void avdc_clear_row(uint8_t row);

uint16_t avdc_get_pointer(uint8_t row, uint8_t col);
uint16_t avdc_get_pointer_cached(uint8_t row, uint8_t col);

void avdc_read_at_pointer(uint16_t addr, uint8_t *chr, uint8_t *attr);

void avdc_write_at_pointer(uint16_t addr, uint8_t chr, uint8_t attr);
void avdc_write_str_at_pointer(uint16_t addr, uint8_t *str, uint8_t *attr);
void avdc_write_str_at_pointer_pos(uint8_t row, uint8_t col, uint8_t *str, uint8_t *attr);

void avdc_set_cursor(uint8_t row, uint8_t col);

void avdc_write_at_cursor(uint8_t chr, uint8_t attr);
void avdc_write_str_at_cursor(uint8_t *str, uint8_t *attr);
void avdc_write_str_at_cursor_pos(uint8_t row, uint8_t col, uint8_t *str, uint8_t *attr);

void avdc_define_glyph(uint8_t char_code, uint8_t *char_data);

void avdc_write_glyphs_at_pointer(uint16_t addr, uint8_t glyph_count, uint8_t *glyphs, uint8_t attr);
void avdc_write_glyphs_at_pointer_pos(uint8_t row, uint8_t col, uint8_t glyph_count, uint8_t *glyphs, uint8_t attr);
void avdc_write_glyphs_at_cursor(uint8_t glyph_count, uint8_t *glyphs, uint8_t attr);
void avdc_write_glyphs_at_cursor_pos(uint8_t row, uint8_t col, uint8_t glyph_count, uint8_t *glyphs, uint8_t attr);

void avdc_done();
void avdc_done_ex(bool reset_avdc);

#endif