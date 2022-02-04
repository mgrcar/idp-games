#include <stdlib.h>
#include "avdc.h"
#include "utils.h"

uint8_t _rows = 26;
uint8_t _cols = 132;
bool _restore = false;
uint16_t _row_addr[128]; // AVDC supports up to 128 rows

uint16_t _system_row_table[26];

uint8_t _init_str_80[] = {
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

uint8_t _init_str_132[] = {
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

// init / done

void _save_row_table() {
    for (int row = 0; row < 26; row++) {
        _system_row_table[row] = avdc_get_pointer(row, 0);
    }
}

void avdc_init() {
    _save_row_table();
    avdc_cursor_off();
    avdc_purge();
    // cache row pointers
    for (uint8_t i = 0; i < 26; i++) {
        _row_addr[i] = avdc_get_pointer(i, 0);
        if (_row_addr[i] == 0xFFFF) { // we are in Matej's emulator, so we do something that works until you scroll the screen
            for (uint8_t j = 0; j < 26; j++) {
                _row_addr[j] = j * 132 + 450;
            }
            break;
        }
    }
}

void avdc_init_ex(avdc_mode mode, uint8_t txt_attr_reg, uint8_t *init_str) {
    // TODO: don't reset if we are already there
    _save_row_table();
    avdc_reset(mode, txt_attr_reg, init_str);
    _restore = true;
}

void avdc_done() {
    if (_restore) {
        avdc_reset(AVDC_MODE_80, 0, NULL); // TODO: check which mode to restore to
    } else {
        avdc_purge();
    }
}

// init / done aux

void avdc_purge() {
    uint16_t addr = _rows * 2; // skip the row table
    while (addr <= 0xFFF) {
        avdc_wait_access();
        avdc_wait_ready();
        // set cursor
        AVDC_CUR_LWR = LO(addr);
        AVDC_CUR_UPR = HI(addr);
        // set pointer
        addr += 0xFF;
        if (addr > 0xFFF) { addr = 0xFFF; }
        AVDC_CMD = AVDC_CMD_SET_PTR_REG;
        AVDC_INIT = LO(addr);
        AVDC_INIT = HI(addr);
        // write cursor to pointer
        AVDC_CHR = ' ';
        AVDC_ATTR = 0;
        AVDC_CMD = AVDC_CMD_WRITE_C2P;
        // wait for C2P to complete
        avdc_wait_long_command();
        addr++;
    }
}

void avdc_reset(avdc_mode mode, uint8_t custom_txt_attr_reg, uint8_t *custom_init_str) {
    // prep work (create "safe" row table)
    avdc_cursor_off();
    uint8_t old_rows = _rows;
    uint8_t *init_str = mode == AVDC_MODE_80 ? _init_str_80 : (mode == AVDC_MODE_132 ? _init_str_132 : custom_init_str);
    _rows = (init_str[4] & 127) + 1;
    _cols = init_str[5] + 1;
    uint8_t safe_table_rows = old_rows > _rows ? old_rows : _rows;
    avdc_wait_access();
    avdc_wait_ready();
    uint16_t addr = safe_table_rows * 2; // skip the row table
    AVDC_CUR_LWR = LO(addr);
    AVDC_CUR_UPR = HI(addr);
    addr += 0xFF; // max AVDC row length is 256
    AVDC_CMD = AVDC_CMD_SET_PTR_REG;
    AVDC_INIT = LO(addr);
    AVDC_INIT = HI(addr);
    AVDC_CHR = ' ';
    AVDC_ATTR = 0;
    AVDC_CMD = AVDC_CMD_WRITE_C2P;
    avdc_wait_long_command();
    // write table
    avdc_set_cursor_addr(0);
    for (uint8_t row = 0; row < safe_table_rows; row++) {
        avdc_write_addr_at_cursor(safe_table_rows * 2);
    }
    // reset AVDC
    avdc_wait_access();
    avdc_wait_ready();
    // WARNME: In the trace, reset is called twice. The spec says you only need to call it twice on power-up. So we call it only once here.
    AVDC_CMD = AVDC_CMD_RESET;
    // set common text attributes
    AVDC_COMMON_TXT_ATTR = mode == AVDC_MODE_80 ? 0x65 : (mode == AVDC_MODE_132 ? 0xC4 : custom_txt_attr_reg);
    // wait for V blanking
    while ((AVDC_GDP_STATUS & 2) != 2);
    while ((AVDC_GDP_STATUS & 2) == 2);
    // set Screen Start 2
    AVDC_SCREEN_START_2_LOWER = 0;
    AVDC_SCREEN_START_2_UPPER = 0;
    // write to init registers
    AVDC_CMD = AVDC_CMD_SET_REG_0;
    for (uint8_t i = 0; i < 10; i++) {
        AVDC_INIT = init_str[i];
    }
    // for some reason, set Screen Start 2 again
    // WARNME: is this really needed?
    AVDC_SCREEN_START_2_LOWER = 0;
    AVDC_SCREEN_START_2_UPPER = 0;
    // display on (turned off by reset)
    AVDC_CMD = AVDC_CMD_DISPLAY_ON;
    // wrap-up (purge and write new row table)
    avdc_cursor_off();
    avdc_purge();
    // write table
    avdc_set_cursor_addr(0);
    if (mode == AVDC_MODE_CUSTOM) {
        uint16_t row_addr = _rows * 2;
        for (uint8_t table_row = 0; table_row < _rows; table_row++) {
            avdc_write_addr_at_cursor(row_addr);
            _row_addr[table_row] = row_addr;
            row_addr += _cols;
        }
    } else {
        for (uint8_t table_row = 0; table_row < 26; table_row++) {
            avdc_write_addr_at_cursor(_row_addr[table_row] = _system_row_table[table_row]);
        }
    }
}

uint8_t *avdc_create_init_str(avdc_mode base, uint8_t cols, uint8_t rows, uint8_t char_width, uint8_t char_height, uint8_t *txt_attr, uint8_t *init_str) {
    uint8_t *base_init_str = base == AVDC_MODE_80 ? _init_str_80 : _init_str_132;
    for (uint8_t i = 0; i < 10; i++) {
        init_str[i] = base_init_str[i];
    }
    *txt_attr = base == AVDC_MODE_80 ? 0x65 : 0xC4;
    init_str[0] = (init_str[0] & 135) | ((char_height - 1) << 3); // adjust char height IR0 (?XXXX???)
    init_str[4] = (init_str[4] & 128) | (rows - 1); // adjust rows IR4 (?XXXXXXX)
    init_str[5] = cols - 1; // adjust cols IR5 (XXXXXXXX)
    // adjust char width (?XX?????: 00=10, 11=9, 10=8, 01=7)
    *txt_attr &= 159;
    if (char_width < 10) {
        *txt_attr |= (char_width - 6) << 5;
    }
    return init_str;
}

void avdc_write_addr_at_cursor(uint16_t addr) {
    avdc_wait_access();
    avdc_wait_ready();
    AVDC_CHR = LO(addr);
    AVDC_ATTR = 0;
    AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
    avdc_wait_ready();
    AVDC_CHR = HI(addr);
    AVDC_ATTR = 0;
    AVDC_CMD = AVDC_CMD_WRITE_AT_CUR;
}

// wait access

void avdc_wait_access() {
    uint8_t status = 0;
    while ((status & AVDC_ACCESS_FLAG) == 0) {
        status = AVDC_ACCESS;
    }
    while ((status & AVDC_ACCESS_FLAG) != 0) {
        status = AVDC_ACCESS;
    }
}

void avdc_wait_ready() {
    uint8_t status = 0;
    while ((status & AVDC_STATUS_READY) == 0) {
        status = AVDC_STATUS;
    }
}

void avdc_wait_long_command() {
    uint8_t status = 0;
    while ((status & AVDC_STATUS_READY) == 0) {
        avdc_wait_access();
        status = AVDC_STATUS;
    }
}

// cursor on / off

void avdc_cursor_off() {
    avdc_wait_access();
    AVDC_CMD = AVDC_CMD_CUR_OFF;
}

void avdc_cursor_on() {
    avdc_wait_access();
    AVDC_CMD = AVDC_CMD_CUR_ON;
}

// clear screen

void avdc_clear_screen() {
    for (uint8_t row = 0; row < _rows; row++) {
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
    addr += _cols - 1;
    AVDC_CMD = AVDC_CMD_SET_PTR_REG;
    AVDC_INIT = LO(addr);
    AVDC_INIT = HI(addr);
    // write cursor to pointer
    AVDC_CHR = ' ';
    AVDC_ATTR = 0;
    AVDC_CMD = AVDC_CMD_WRITE_C2P;
    // wait for C2P to complete
    avdc_wait_long_command();
}

// read at pointer

uint16_t avdc_get_pointer(uint8_t row, uint8_t col) {
    uint16_t _row_addr;
    uint8_t dummy;
    avdc_read_at_pointer(row * 2, &(LO(_row_addr)), &dummy);
    avdc_read_at_pointer(row * 2 + 1, &(HI(_row_addr)), &dummy);
    return _row_addr + col;
}

uint16_t avdc_get_pointer_cached(uint8_t row, uint8_t col) {
    return _row_addr[row] + col;
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
}

// write at pointer

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

// write at cursor

void avdc_set_cursor(uint8_t row, uint8_t col) {
    avdc_wait_access();
    avdc_wait_ready(); // In the independent mode, the ROFLG bit of the status register should be checked for the ready state (bit 5 equal to a logic one) before writing to the cursor address registers.
    uint16_t addr;
    addr = avdc_get_pointer_cached(row, col);
    AVDC_CUR_LWR = LO(addr);
    AVDC_CUR_UPR = HI(addr);
}

void avdc_set_cursor_addr(uint16_t addr) {
    avdc_wait_access();
    avdc_wait_ready();
    AVDC_CUR_LWR = LO(addr);
    AVDC_CUR_UPR = HI(addr);
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

// define / write glyphs

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