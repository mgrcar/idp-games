#include <stdlib.h>
#include "avdc.h"
#include "utils.h"

uint8_t avdc_rows = 26;
uint16_t row_addr[26];

uint8_t avdc_init_str_80[] = { 
    0xD0, // IR0: 1 (DOUBLE HT/WD ON) / 1010 (11 SCAN LINES PER CHAR ROW) / 0 (SYNC = VSYNC) / 00 (BUFFER MODE = INDEPENDENT)
    0x2F, // IR1: 0 (NON-INTERLACED) / 0101111 (EQUALIZING CONSTANT)
    0x8D, // IR2: 1 (ROW TABLE ON) / 0001 (HSYNC WIDTH) / 101 (H BACK PORCH)
    0x05, // IR3: 000 (V FRONT PORCH) / 00101 (V BACK PORCH)
    0x99, // IR4: 1 (BLINK RATE) / 0011001 (ACTIVE CHAR ROWS PER SCREEN = 25 + 1)
    0x4F, // IR5: 01001111 (ACTIVE CHARS PER ROW = 79 + 1)
    0x0A, // IR6: 0000 (FIRST LINE OF CURSOR = SCAN LINE 0) / 1010 (LAST LINE OF CURSOR = SCAN LINE 10 STARTING AT 0)
    0xEA, // IR7: 11 (VSYNC WIDTH) / 1 (CURSOR BLINK ON) / 0 (CURSOR RATE) / 1010 (UNDERLINE POSITION = SCAN LINE 10 STARTING AT 0)
    0x00, // IR8: DISPLAY BUFFER 1ST ADDRESS LSB'S
    0x30  // IR9: 0011 (DISPLAY BUFFER LAST ADDR) / 0000 (DISPLAY BUFFER 1ST ADDRESS MSB'S)
};

uint8_t avdc_init_str_132[] = { 
    0xD0, // IR0: 1 (DOUBLE HT/WD ON) / 1010 (11 SCAN LINES PER CHAR ROW) / 0 (SYNC = VSYNC) / 00 (BUFFER MODE = INDEPENDENT)
    0x3E, // IR1: 0 (NON-INTERLACED) / 0111110 (EQUALIZING CONSTANT)
    0xBF, // IR2: 1 (ROW TABLE ON) / 0111 (HSYNC WIDTH) / 111 (HORIZ BACK PORCH)
    0x05, // IR3: 000 (V FRONT PORCH) / 00101 (V BACK PORCH)
    0x99, // IR4: 1 (BLINK RATE) / 0011001 (ACTIVE CHAR ROWS PER SCREEN = 25 + 1)
    0x83, // IR5: 10000011 (ACTIVE CHARACTERS PER ROW = 131 + 1)
    0x0B, // IR6: 0000 (FIRST LINE OF CURSOR) / 1011 (LAST LINE OF CURSOR = SCAN LINE 11 STARTING AT 0) 
    0xEA, // IR7: 11 (VSYNC WIDTH) / 1 (CURSOR BLINK ON) / 0 (CURSOR RATE) / 1010 (UNDERLINE POSITION = SCAN LINE 10 STARTING AT 0)
    0x00, // IR8: DISPLAY BUFFER 1ST ADDRESS LSB'S
    0x30  // IR9: 0011 (DISPLAY BUFFER LAST ADDR) / 0000 (DISPLAY BUFFER 1ST ADDRESS MSB'S)
};

void avdc_init() {
    for (uint8_t i = 0; i < avdc_rows; i++) {
        row_addr[i] = avdc_get_pointer(i, 0);
        if (row_addr[i] == 0xFFFF) { // we are in Matej's emulator, so we do something that works until you scroll the screen
            for (uint8_t j = 0; j < avdc_rows; j++) {
                row_addr[j] = j * 132 + 450;
            }
            break;
        }
    }
    avdc_cursor_off();
    avdc_clear_screen();
}

void avdc_init_ex(avdc_mode mode, uint8_t custom_txt_attr_reg, uint8_t *custom_init_str) {
    avdc_reset(mode, custom_txt_attr_reg, custom_init_str);
    avdc_init();
}

void avdc_reset(avdc_mode mode, uint8_t custom_txt_attr_reg, uint8_t *custom_init_str) {
    avdc_wait_access();
    avdc_wait_ready();
    // reset AVDC
    // WARNME: In the trace, reset is called twice. The spec says you only need to call it twice on power-up. So we call it only once here.
    AVDC_CMD = AVDC_CMD_RESET;
    // set common text attributes
    AVDC_COMMON_TXT_ATTR = mode == AVDC_MODE_80 ? 0x65 : (mode == AVDC_MODE_132 ? 0xC4 : custom_txt_attr_reg);
    // wait for V blanking
    while ((AVDC_GDP_STATUS & 2) != 2);
    while ((AVDC_GDP_STATUS & 2) == 2);
    // set screen start 2
    AVDC_SCREEN_START_2_LOWER = 0;
    AVDC_SCREEN_START_2_UPPER = 0;
    // write to init registers
    AVDC_CMD = AVDC_CMD_SET_REG_0;
    uint8_t *init_str = mode == AVDC_MODE_80 ? avdc_init_str_80 : (mode == AVDC_MODE_132 ? avdc_init_str_132 : custom_init_str);
    for (uint8_t i = 0; i < 10; i++) {
        AVDC_INIT = init_str[i];
    }   
    // for some reason, set screen start 2 again // WARNME: is this really needed?
    AVDC_SCREEN_START_2_LOWER = 0;
    AVDC_SCREEN_START_2_UPPER = 0;
    // display on (turned off by reset)
    AVDC_CMD = AVDC_CMD_DISPLAY_ON;
    // set avdc_rows
    avdc_rows = (init_str[4] & 127) + 1;
}

void avdc_wait_access() {
    uint8_t status = 0;
    while ((status & AVDC_ACCESS_FLAG) == 0) {
        status = AVDC_ACCESS;
    }
    EI;
    while ((status & AVDC_ACCESS_FLAG) != 0) {
        status = AVDC_ACCESS;
    }
    DI;
}

void avdc_wait_ready() {
    uint8_t status = 0;
    while ((status & AVDC_STATUS_READY) == 0) {
        status = AVDC_STATUS;
    }
}

void avdc_cursor_off() {
    avdc_wait_access();
    AVDC_CMD = AVDC_CMD_CUR_OFF;
    EI;
}

void avdc_cursor_on() {
    avdc_wait_access();
    AVDC_CMD = AVDC_CMD_CUR_ON;
    EI;
}

void avdc_clear_screen() {
    for (uint8_t row = 0; row < avdc_rows; row++) {
        avdc_clear_row(row);
    }
}

void avdc_clear_row(uint8_t row) {
    avdc_wait_access();
    avdc_wait_ready();
    uint16_t addr = avdc_get_pointer_cached(row, 0); 
    // set cursor
    AVDC_CUR_LWR = LO(addr);
    AVDC_CUR_UPR = HI(addr);
    // set pointer
    addr += 132; // WARNME: off-by-one error? 
    AVDC_CMD = AVDC_CMD_SET_PTR_REG;
    AVDC_INIT = LO(addr);
    AVDC_INIT = HI(addr);
    // write cursor to pointer
    AVDC_CHR = ' '; 
    AVDC_ATTR = 0;
    AVDC_CMD = AVDC_CMD_WRITE_C2P;
    EI;
}

uint16_t avdc_get_pointer(uint8_t row, uint8_t col) {
    uint16_t row_addr;
    uint8_t dummy;
    avdc_read_at_pointer(row * 2, &(LO(row_addr)), &dummy);
    avdc_read_at_pointer(row * 2 + 1, &(HI(row_addr)), &dummy);
    return row_addr + col;
}

uint16_t avdc_get_pointer_cached(uint8_t row, uint8_t col) {
    return row_addr[row] + col;
}

void avdc_read_at_pointer(uint16_t addr, uint8_t *chr, uint8_t *attr) { 
    avdc_wait_access();
    // CPU checks RDFLG status bit to assure that any delayed commands have been completed.
    avdc_wait_ready();
    // CPU writes addr into pointer registers.
    AVDC_CMD = AVDC_CMD_SET_PTR_REG;
    AVDC_INIT = LO(addr);
    AVDC_INIT = HI(addr);
    // CPU issues 'read at pointer' command. AVDC generates control signals and outputs specified addr to perform requested operation. Data is copied from memory to the interface latch and AVDC sets RDFLG status to indicate that the read is completed.
    AVDC_CMD = AVDC_CMD_READ_AT_PTR;
    // CPU checks RDFLG status to see if operation is completed.
    avdc_wait_ready();
    // CPU reads data from interface latch.
    *chr = AVDC_CHR;
    *attr = AVDC_ATTR;
    EI;
}

void avdc_write_at_pointer(uint16_t addr, uint8_t chr, uint8_t attr) {
    avdc_wait_access();
    // CPU checks RDFLG status bit to assure that any delayed commands have been completed.
    avdc_wait_ready();
    // CPU writes addr into pointer registers.
    AVDC_CMD = AVDC_CMD_SET_PTR_REG;
    AVDC_INIT = LO(addr);
    AVDC_INIT = HI(addr);
    // CPU loads data to be written to display memory into the interface latch.
    AVDC_CHR = chr;
    AVDC_ATTR = attr;
    // CPU issues 'write at pointer' command. AVDC generates control signals and outputs specified addr to perform requested operation. Data is copied from the interface latch into the memory. AVDC sets RDFLG status to indicate that the write is completed.
    AVDC_CMD = AVDC_CMD_WRITE_AT_PTR;
    EI;
}

void avdc_write_str_at_pointer(uint16_t addr, uint8_t *str, uint8_t *attr) { 
    if (attr) {
        while (*str != 0) {
            avdc_write_at_pointer(addr, *str, *attr);
            addr++;
            str++;
            attr++; 
        }
    } else {
        while (*str != 0) {
            avdc_write_at_pointer(addr, *str, AVDC_DEFAULT_ATTR);
            addr++;
            str++;
        }
    }
}

void avdc_write_str_at_pointer_pos(uint8_t row, uint8_t col, uint8_t *str, uint8_t *attr) { 
    avdc_write_str_at_pointer(avdc_get_pointer_cached(row, col), str, attr);
}

void avdc_set_cursor(uint8_t row, uint8_t col) {
    avdc_wait_access();
    avdc_wait_ready(); // In the independent mode, the ROFLG bit of the status register should be checked for the ready state (bit 5 equal to a logic one) before writing to the cursor address registers.
    uint16_t addr;
    addr = avdc_get_pointer_cached(row, col);
    AVDC_CUR_LWR = LO(addr);
    AVDC_CUR_UPR = HI(addr);
    EI;
}

void avdc_write_at_cursor(uint8_t chr, uint8_t attr) {
    avdc_wait_access();
    // CPU checks RDFLG status bit to assure that any delayed commands have been completed.
    avdc_wait_ready();
    // CPU loads data to be written to display memory into the interface latch.
    AVDC_CHR = chr;
    AVDC_ATTR = attr;
    // CPU issues 'write at cursor' command. AVDC generates control signals and outputs specified addr to perform requested operation. Data is copied from the interface latch into the memory. AVDC sets RDFLG status to indicate that the write is completed.
    AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
    EI;
}

void avdc_write_str_at_cursor(uint8_t *str, uint8_t *attr) {
    if (attr) {
        while (*str != 0) {
            avdc_write_at_cursor(*str, *attr);
            str++;
            attr++; 
        }
    } else {
        while (*str != 0) {
            avdc_write_at_cursor(*str, AVDC_DEFAULT_ATTR);
            str++;
        }
    }
}

void avdc_write_str_at_cursor_pos(uint8_t row, uint8_t col, uint8_t *str, uint8_t *attr) {
    avdc_set_cursor(row, col);
    avdc_write_str_at_cursor(str, attr);
}

void avdc_define_glyph(uint8_t char_code, uint8_t *char_data) {
    uint16_t addr = 8 * 1024 + char_code * 16;
    for (uint8_t i = 0; i < 16; i++) {
        avdc_write_at_pointer(addr++, char_data[i], 0);
    }
}

void avdc_write_glyphs_at_pointer(uint16_t addr, uint8_t glyph_count, uint8_t *glyphs, uint8_t attr) {
    attr |= AVDC_ATTR_UDG;
    for (uint8_t i = 0; i < glyph_count; i++) {
        avdc_write_at_pointer(addr, *glyphs, attr);
        addr++;
        glyphs++;
    }
}

void avdc_write_glyphs_at_pointer_pos(uint8_t row, uint8_t col, uint8_t glyph_count, uint8_t *glyphs, uint8_t attr) {
    avdc_write_glyphs_at_pointer(avdc_get_pointer_cached(row, col), glyph_count, glyphs, attr);
}

void avdc_write_glyphs_at_cursor(uint8_t glyph_count, uint8_t *glyphs, uint8_t attr) {
    attr |= AVDC_ATTR_UDG;
    for (uint8_t i = 0; i < glyph_count; i++) {
        avdc_write_at_cursor(*glyphs, attr);
        glyphs++;
    }
}

void avdc_write_glyphs_at_cursor_pos(uint8_t row, uint8_t col, uint8_t glyph_count, uint8_t *glyphs, uint8_t attr) {
    avdc_set_cursor(row, col);
    avdc_write_glyphs_at_cursor(glyph_count, glyphs, attr);
}

void avdc_done() {
    avdc_clear_screen();
}

void avdc_done_ex(bool reset_avdc) {
    avdc_done();
    if (reset_avdc) {
        // TODO: determine the mode to reset to
        avdc_reset(AVDC_MODE_80, 0, NULL);
    }
}