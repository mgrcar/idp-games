#include "avdc.h"
#include "utils.h"

uint16_t row_addr[26];

void avdc_init() {
    for (uint8_t i = 0; i < 26; i++) {
        row_addr[i] = avdc_get_pointer(i, 0);
        if (row_addr[i] == 0xFFFF) { // we are in Matej's emulator, so we do something that works until you scroll the screen
            for (uint8_t j = 0; j < 26; j++) {
                row_addr[j] = j * 132 + 450;
            }
            break;
        }
    }
    avdc_cursor_off();
    avdc_clear_screen();
}

void avdc_wait_access() { // WARNME: disables interrupts
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
    for (uint8_t row = 0; row < 26; row++) {
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
    addr += 132; 
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

void avdc_done() {
    avdc_clear_screen();
}