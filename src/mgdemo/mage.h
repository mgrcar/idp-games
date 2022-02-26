#ifndef __MAGE_H__
#define __MAGE_H__

#include <stdlib.h>

#define MAGE_ATTR_NORMAL 0x00
#define MAGE_ATTR_BRIGHT 0x40
#define MAGE_ATTR_DIM    0x80

void mage_init();
void mage_done();

void mage_display_off();
void mage_display_on();

void mage_render();
void mage_flag_dirty(uint8_t row, uint8_t col);

void mage_set_block(uint8_t row, uint8_t col, uint8_t val);

#endif