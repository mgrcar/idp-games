#include "gdp.h"
#include "utils.h"

uint8_t gdp_write_page;
uint8_t gdp_display_page;

void gdp_wait_ready() {
    uint8_t status = 0;
    while ((status & GDP_STATUS_READY) == 0) {
        status = GDP_STATUS;
    }
}

void gdp_init() {
    gdp_wait_ready();
    GFX_COMMON = 0x03; // format: 00, write mode: 0, pages: 11
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_DOWN | GDP_TOOL_PEN;
    GDP_CTRL_2 = GDP_STYLE_NORMAL;
    GDP_CHAR_SIZE = 0x21;
    GDP_CMD = GDP_CMD_CLS; 
    gdp_wait_ready();
    GFX_COMMON = 0x00; // format: 00, write mode: 0, pages: 00
    GDP_CMD = GDP_CMD_CLS; 
    gdp_write_page = 0;
    gdp_display_page = 0;
    GDP_SCROLL = 0;
    if (sys_is_emu()) {
        __asm
            ld  hl, #call_gdp_line_dx_pos_dotted
            ld  bc, #_gdp_line_dx_pos_dotted_emu
            // overwrite "call _gdp_line_dx_pos_dotted" with "call _gdp_line_dx_pos_dotted_emu"
            inc hl
            ld  (hl), c
            inc hl
            ld  (hl), b
        __endasm;
    }
}

void gdp_cls() {
    gdp_wait_ready();
    GDP_CMD = GDP_CMD_CLS;  
}

void gdp_set_xy(uint16_t x, uint8_t y) {
    gdp_wait_ready();
    GDP_X_HI = HI(x);
    GDP_X_LO = LO(x);
    GDP_Y_HI = 0;
    GDP_Y_LO = ~y;
}

void gdp_line_dx_pos(gdp_tool tool, gdp_style style, uint8_t dx) {
    gdp_wait_ready();
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_DOWN | tool;
    GDP_CTRL_2 = style; 
    GDP_DX = dx;
    GDP_CMD = GDP_CMD_LINE_DX_POS;
}

void gdp_line_delta(gdp_tool tool, gdp_style style, uint8_t dx, uint8_t dy, gdp_delta_sign delta_sign) { 
    gdp_wait_ready();
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_DOWN | tool;
    GDP_CTRL_2 = style; 
    GDP_DX = dx;
    GDP_DY = dy;
    GDP_CMD = GDP_CMD_LINE_DELTA | delta_sign;
}

void gdp_line_dx_pos_dotted(uint8_t dx) {
    gdp_line_dx_pos(GDP_TOOL_ERASER, GDP_STYLE_NORMAL, dx);
    gdp_wait_ready();
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_DOWN | GDP_TOOL_PEN;
    GDP_CTRL_2 = GDP_STYLE_DOTTED;
    GDP_DX = dx;
    GDP_CMD = GDP_CMD_LINE_DX_NEG;
    gdp_wait_ready();
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_UP | GDP_TOOL_PEN;
    GDP_CTRL_2 = GDP_STYLE_NORMAL;
    GDP_DX = dx;
    GDP_CMD = GDP_CMD_LINE_DX_POS;
}

void gdp_line_dx_pos_dotted_emu(uint8_t dx) {
    gdp_line_dx_pos(GDP_TOOL_ERASER, GDP_STYLE_NORMAL, dx);
    gdp_wait_ready();
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_UP | GDP_TOOL_PEN;
    GDP_CTRL_2 = GDP_STYLE_NORMAL;
    GDP_DX = dx;
    GDP_CMD = GDP_CMD_LINE_DX_NEG;
    gdp_line_dx_pos(GDP_TOOL_PEN, GDP_STYLE_DOTTED, dx);
}

void gdp_write(gdp_tool tool, uint8_t *text) {
    uint8_t i = 0;
    gdp_wait_ready();
    GDP_CTRL_1 = GDP_CTRL_1_TOOL_DOWN | tool;
    while (text[i] != 0) {
        gdp_wait_ready();
        GDP_CMD = text[i++];
    }
}

void gdp_set_mode(gdp_mode mode) {
    gdp_wait_ready();
    uint8_t pages = GFX_COMMON;
    pages &= 3; // take b1 and b0 (write and display page WD)
    GFX_COMMON = mode | pages; // format: 00, write mode: M, pages: WD
}

void gdp_set_write_page(uint8_t page) {
    gdp_wait_ready();
    uint8_t val = GFX_COMMON;
    val &= 5; // take b3 and b0 (write mode M and display page D)
    val |= page << 1;
    GFX_COMMON = val; // format: 00, write mode: M, pages: WD   
    gdp_write_page = page;
}

void gdp_set_display_page(uint8_t page) {
    gdp_wait_ready();
    uint8_t val = GFX_COMMON;
    val &= 6; // take b3 and b1 (write mode M and write page W)
    val |= page;
    GFX_COMMON = val; // format: 00, write mode: M, pages: WD
    gdp_display_page = page;
}

void gdp_line_dx_pos_dotted_proxy(uint8_t dx) {
    __asm   
        ld   hl, #2
        add  hl, sp
        ld   a, (hl)
        push af
        inc  sp
    call_gdp_line_dx_pos_dotted:
        call _gdp_line_dx_pos_dotted
        inc  sp
    __endasm;
}

void gdp_draw_row_xor_mask(uint8_t *mask_row) { 
    uint8_t len = mask_row[0];
    uint8_t pen_up = GDP_CTRL_1_TOOL_UP;
    int8_t offset = 0;
    for (uint8_t i = 1; i <= len; i++) {
        uint8_t rl = mask_row[i] << 1;
        if (rl > 0 || offset >= 0) {
            gdp_wait_ready();
            GDP_CTRL_1 = pen_up | GDP_TOOL_PEN;
            GDP_CTRL_2 = GDP_STYLE_NORMAL;
            GDP_DX = rl + offset;
            GDP_CMD = GDP_CMD_LINE_DX_POS;
            offset = offset >= 0 ? -1 : 1;
        } else {
            offset = offset >= 0 ? -1 : 0;
        }
        pen_up = ~pen_up & 1;
    }
}

void gdp_draw_row_sprite(uint8_t *sprite_row) { // TODO: asm implementation?
    uint8_t b;
    uint8_t len = sprite_row[0];
    if (len > 0) {
        uint8_t q = (sprite_row[1] >> 7) & 1;
        for (uint8_t i = 1; i < len; i++) {
            b = sprite_row[i];
            q = (q << 1) | ((sprite_row[i + 1] >> 7) & 1);
            gdp_wait_ready();
            GDP_CTRL_1 = ((b >> 7) & 1) | ((b >> 5) & 2); 
            GDP_CTRL_2 = GDP_STYLE_NORMAL;
            GDP_DX = (((b & 63) << 1) | ((q >> 2) & (~q >> 1) & 1)) - ((q >> 1) & ~q & 1);
            GDP_CMD = GDP_CMD_LINE_DX_POS;
        }
        b = sprite_row[len];
        gdp_wait_ready();
        GDP_CTRL_1 = ((b >> 7) & 1) | ((b >> 5) & 2); 
        GDP_CTRL_2 = GDP_STYLE_NORMAL;
        GDP_DX = ((b & 63) << 1) - 1;
        GDP_CMD = GDP_CMD_LINE_DX_POS;
    }
}

void gdp_erase_row_sprite(uint8_t *sprite_row) { // TODO: asm implementation?
    uint8_t b;
    uint8_t len = sprite_row[0];
    if (len > 0) {
        uint8_t q = (sprite_row[1] >> 7) & 1;
        for (uint8_t i = 1; i < len; i++) {
            b = sprite_row[i];
            q = (q << 1) | ((sprite_row[i + 1] >> 7) & 1);
            gdp_wait_ready();
            GDP_CTRL_1 = (((b >> 7) & 1) | ((b >> 5) & 2)) & 0xFD;
            GDP_CTRL_2 = GDP_STYLE_NORMAL;
            GDP_DX = (((b & 63) << 1) | ((q >> 2) & (~q >> 1) & 1)) - ((q >> 1) & ~q & 1);
            GDP_CMD = GDP_CMD_LINE_DX_POS;
        }
        b = sprite_row[len];
        gdp_wait_ready();
        GDP_CTRL_1 = (((b >> 7) & 1) | ((b >> 5) & 2)) & 0xFD;
        GDP_CTRL_2 = GDP_STYLE_NORMAL;
        GDP_DX = ((b & 63) << 1) - 1;
        GDP_CMD = GDP_CMD_LINE_DX_POS;
    }
}

void gdp_draw_row(uint8_t *image_row) {
    uint8_t len = image_row[0];
    gdp_tool tool = GDP_TOOL_PEN; // tool can be GDP_TOOL_ERASER (0) or GDP_TOOL_PEN (2)
    for (uint8_t i = 1; i < len; i++) {
        uint8_t b = image_row[i];
        gdp_style style = (b & 64) >> 6; // resolves to GDP_STYLE_NORMAL (0) or GDP_STYLE_DOTTED (1)
        if ((b & 128) == 0) {
            if (style == GDP_STYLE_DOTTED) {
                gdp_line_dx_pos_dotted_proxy((b & 63) << 1);
            } else {
                gdp_line_dx_pos(tool, style, (b & 63) << 1);
            }
        } else {
            gdp_style _style = (tool >> 1) & style; // NOTE: style can be dotted only when pen is selected
            if (_style == GDP_STYLE_DOTTED) {
                gdp_line_dx_pos_dotted_proxy(((b >> 3) & 7) << 1); 
            } else {
                gdp_line_dx_pos(tool, _style, ((b >> 3) & 7) << 1); 
            }
            tool = ~tool & 2; // equivalent to tool = (tool == GDP_TOOL_PEN) ? GDP_TOOL_ERASER : GDP_TOOL_PEN
            _style = (tool >> 1) & style;
            if (_style == GDP_STYLE_DOTTED) {
                gdp_line_dx_pos_dotted_proxy((b & 7) << 1); 
            } else {
                gdp_line_dx_pos(tool, _style, (b & 7) << 1); 
            }
        }
        tool = ~tool & 2;  
    }
    // the last RL is decreased by 1
    uint8_t b = image_row[len];
    gdp_style style = (b & 64) >> 6; 
    if ((b & 128) == 0) {
        if (style == GDP_STYLE_DOTTED) {
            gdp_line_dx_pos_dotted_proxy(((b & 63) << 1) - 1);
        } else {
            gdp_line_dx_pos(tool, style, ((b & 63) << 1) - 1);
        }
    } else {
        gdp_style _style = (tool >> 1) & style; 
        if (_style == GDP_STYLE_DOTTED) {
            gdp_line_dx_pos_dotted_proxy(((b >> 3) & 7) << 1); 
        } else {
            gdp_line_dx_pos(tool, _style, ((b >> 3) & 7) << 1); 
        }
        tool = ~tool & 2; 
        _style = (tool >> 1) & style;
        if (_style == GDP_STYLE_DOTTED) {
            gdp_line_dx_pos_dotted_proxy(((b & 7) << 1) - 1); 
        } else {
            gdp_line_dx_pos(tool, _style, ((b & 7) << 1) - 1); 
        }
    }
}

void gdp_draw(uint8_t **image, uint8_t last_row_idx, uint16_t x, uint8_t y) {
    for (int i = 0, j = last_row_idx; i <= j; i++, j--) {
        gdp_set_xy(x, i + y);
        gdp_draw_row(image[i]); 
        if (i != j) {
            gdp_set_xy(x, j + y);
            gdp_draw_row(image[j]); 
        }
    }
}

void gdp_draw_xor_mask(uint8_t **mask, uint8_t last_row_idx, uint16_t x, uint8_t y) {
    for (int i = 0, j = last_row_idx; i <= j; i++, j--) {
        gdp_set_xy(x, i + y);
        gdp_draw_row_xor_mask(mask[i]); 
        if (i != j) {
            gdp_set_xy(x, j + y);
            gdp_draw_row_xor_mask(mask[j]); 
        }
    }
}

void gdp_draw_sprite(uint8_t **sprite, uint8_t last_row_idx, uint16_t x, uint8_t y) {
    for (int i = 0, j = last_row_idx; i <= j; i++, j--) {
        gdp_set_xy(x, i + y);
        gdp_draw_row_sprite(sprite[i]); 
        if (i != j) {
            gdp_set_xy(x, j + y);
            gdp_draw_row_sprite(sprite[j]); 
        }
    }
}

void gdp_erase_sprite(uint8_t **sprite, uint8_t last_row_idx, uint16_t x, uint8_t y) {
    for (int i = 0, j = last_row_idx; i <= j; i++, j--) {
        gdp_set_xy(x, i + y);
        gdp_erase_row_sprite(sprite[i]); 
        if (i != j) {
            gdp_set_xy(x, j + y);
            gdp_erase_row_sprite(sprite[j]); 
        }
    }
}

void gdp_draw_rect(gdp_tool tool, uint16_t x, uint8_t y, uint8_t w, uint8_t h) { // TODO: asm implementation?
    gdp_set_xy(x, y + h - 1);
    gdp_line_delta(tool, GDP_STYLE_NORMAL, 0, h - 1, GDP_DELTA_SIGN_DY_NEG);
    gdp_line_dx_pos(tool, GDP_STYLE_NORMAL, w - 1);
    gdp_line_delta(tool, GDP_STYLE_NORMAL, 0, h - 1, GDP_DELTA_SIGN_DY_POS);
    gdp_line_delta(tool, GDP_STYLE_NORMAL, w - 2, 0, GDP_DELTA_SIGN_DX_NEG);
    gdp_line_delta(tool, GDP_STYLE_NORMAL, 0, h - 1, GDP_DELTA_SIGN_DY_NEG);
    gdp_set_xy(x + w - 2, y);
    gdp_line_delta(tool, GDP_STYLE_NORMAL, 0, h - 1, GDP_DELTA_SIGN_DY_POS);
}

void gdp_fill_rect(gdp_tool tool, uint16_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (int i = y, j = y + h - 1; i <= j; i++, j--) {
        gdp_set_xy(x, i);
        gdp_line_dx_pos(tool, GDP_STYLE_NORMAL, w - 1);
        if (i != j) {
            gdp_set_xy(x, j);
            gdp_line_dx_pos(tool, GDP_STYLE_NORMAL, w - 1);
        }
    }
}

void gdp_done() {
    gdp_cls();
    // reset scroll
    GDP_SCROLL = 0;
    // clear 2nd page
    // WARNME: leave page 2 dirty? No, erase it but with a filled rect
}