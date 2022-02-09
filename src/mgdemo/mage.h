#ifndef __MAGE_H__
#define __MAGE_H__

#include <stdlib.h>

#define MAGE_ATTR_NORMAL    0x00
#define MAGE_ATTR_BRIGHTER  0x08
#define MAGE_ATTR_BRIGHTEST 0x04

void mage_init();
void mage_done();

void mage_render();
void mage_flag_dirty(uint8_t row, uint8_t col);

void mage_set_block(uint8_t row, uint8_t col, uint8_t bits);
void mage_set_block_attr(uint8_t row, uint8_t col, uint8_t attr);

#endif