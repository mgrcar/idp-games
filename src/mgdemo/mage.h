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
void mage_set_block_raw(uint8_t row, uint16_t addr, uint8_t val, uint8_t attr);
void mage_render(bool interlaced);

void mage_set_row_addr(uint8_t row, uint16_t addr);
void mage_set_row_addr_after(uint16_t addr1, uint16_t addr2);

#endif