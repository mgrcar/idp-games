#ifndef __MAGE_H__
#define __MAGE_H__

#include <stdlib.h>

#define MAGE_ATTR_NORMAL 0x00
#define MAGE_ATTR_BRIGHT 0x10
#define MAGE_ATTR_DIM    0x20

void mage_init();
void mage_done();

void mage_display_off();
void mage_display_on();

void mage_set_block(uint8_t row, uint8_t col, uint8_t val, uint8_t attr);
void mage_set_block_raw(uint16_t addr, uint8_t val, uint8_t attr);
void mage_render(bool interlaced);
void mage_render_range(uint16_t addr, uint16_t addr_end);

void mage_set_row_addr(uint8_t row, uint16_t addr);

void mage_render_blocks(uint8_t count, uint8_t *blocks);
void mage_render_block_at(uint16_t addr, uint8_t ch, uint8_t attr);

#endif