#ifndef PSF_H
#define PSF_H 1

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <basic.h>
#include <gpxwm/gpxwm.h>

__attribute__((hot))
void psf_renderer_init(void* psf_file_base, size_t sz, int screen_w, int screen_h);
__attribute__((hot))
void font_set_colors(uint32_t fg, uint32_t bg);
__attribute__((hot))
void font_put_char_pos(char c, int x, int y);
__attribute__((hot))
void font_put_char(char c);
__attribute__((hot))
int font_prints(const char* s);
__attribute__((hot))
int font_prints_pos(const char* s, int x, int y);

#endif /* PSF_H */