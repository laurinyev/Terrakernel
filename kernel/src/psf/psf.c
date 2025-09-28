#include "psf.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

typedef struct { uint8_t magic[2]; uint8_t mode; uint8_t charsize; } psf1_header;

static psf1_header* psf1;
static uint8_t* psf1_glyphs;
static int font_w = 8;
static int font_h;
static int term_w, term_h;
static int cursor_x = 0;
static int cursor_y = 0;
static uint32_t fg_color = 0xFFFFFFFF;
static uint32_t bg_color = 0x00000000;

__attribute__((hot))
void psf_renderer_init(void* psf_file_base, size_t sz, int screen_w, int screen_h) {
    psf1 = (psf1_header*)psf_file_base;
    psf1_glyphs = (uint8_t*)psf_file_base + sizeof(psf1_header);
    font_h = psf1->charsize;
    term_w = screen_w / font_w;
    term_h = screen_h / font_h;
    cursor_x = cursor_y = 0;
    fg_color = 0xFFFFFFFF;
    bg_color = 0x00000000;
    SCREEN_WIDTH = screen_w;
    SCREEN_HEIGHT = screen_h;
}

__attribute__((hot))
void font_set_colors(uint32_t fg, uint32_t bg) { 
    fg_color = fg; 
    bg_color = bg; 
}

__attribute__((hot))
static void scroll_up(void) {
    int bytes_per_row = font_h * SCREEN_WIDTH * 4;
    uint8_t* fb = (uint8_t*)0x000B8000;
    memmove(fb, fb + bytes_per_row, (term_h - 1) * bytes_per_row);
    for (int y = (term_h - 1) * font_h; y < term_h * font_h; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            PutPx(x, y, bg_color);
    cursor_y = term_h - 1;
}

__attribute__((hot))
void font_put_char_pos(char c, int x, int y) {
    if ((uint8_t)c >= 256) return;

    uint8_t* glyph = psf1_glyphs + ((uint8_t)c) * font_h;
    for (int row = 0; row < font_h; row++) {
        uint8_t byte = glyph[row];
        for (int bit = 0; bit < 8; bit++) {
            uint32_t color = (byte & (1 << (7 - bit))) ? fg_color : bg_color;
            PutPx(x + bit, y + row, color);
        }
    }
}

__attribute__((hot))
void font_put_char(char c) {
    if (c == '\n') { cursor_x = 0; cursor_y++; }
    else if (c == '\r') { cursor_x = 0; }
    else if (c == '\b') { if (cursor_x > 0) cursor_x--; }
    else if (c == '\t') { 
        int spaces = 4 - (cursor_x % 4); 
        for (int i = 0; i < spaces; i++) font_put_char(' '); 
        return;
    } else if ((uint8_t)c < 256) {
        font_put_char_pos(c, cursor_x, cursor_y);
        cursor_x++;
    }

    if (cursor_x >= term_w) { cursor_x = 0; cursor_y++; }
    if (cursor_y >= term_h) scroll_up();
}

__attribute__((hot))
int font_prints(const char* s) {
    int count = 0;
    while (*s) { font_put_char(*s++); count++; }
    return count;
}

__attribute__((hot))
int font_prints_pos(const char* s, int x, int y) {
    int count = 0;
    while (*s) { font_put_char_pos(*s++, x, y); count++; x++; if (x >= term_w) { x = 0; y++; } }
    return count;
}
