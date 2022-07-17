#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdbool.h>
#include <partner.h>

#define LO(x) ((uint8_t *)&x)[0]
#define HI(x) ((uint8_t *)&x)[1]

// timer

__sfr __at 0xA0 CTC_TENTHS_CS; // R: clock, tenths of a centisecond
__sfr __at 0xA1 CTC_HUNDREDS;  // R: clock, hundreds of a second
__sfr __at 0xA2 CTC_SECONDS;   // R: clock, seconds

extern int16_t timer_start;
extern int16_t timer_offset;

int16_t timer();
uint16_t timer_ms();

void timer_reset(int16_t offset);
int16_t timer_diff();
int16_t timer_diff_ex(uint16_t timer_start, uint16_t timer_offset);

// keyboard

#define KBD_STATUS_READY  0x04
#define KBD_CMD_BEEP      0x02
#define KBD_CMD_BEEP_LONG 0x04

__sfr __at 0xD9 KBD_STATUS; // R: status (ready)
__sfr __at 0xD8 KBD_CMD;    // W: command

void kbd_wait_ready();
void kbd_beep(bool long_beep);
char kbd_get_key();

// other

bool sys_is_emu();
int sys_rand();

#endif