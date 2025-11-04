/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
 *
 * swanshell is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * swanshell is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with swanshell. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __OLD_BITMAP_H__
#define __OLD_BITMAP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>

// Expected layout (tile order ABCDEFGH):
//
// AE ; width = 2
// BF ; height = 4
// CG ; bpp = 2 
// DH ; pitch = 64
typedef struct {
    // Arguments
    uint16_t current_pitch;

    // Bitmap configuration
    uint16_t x_pitch; // Bytes
    uint8_t y_pitch; // Bytes (2 for 1/2 bpp, 4 for 4bpp)
    uint8_t x_shift; // 3 for 2/4bpp, 4 for 1bpp
    uint8_t bpp; // Bits per pixel (1, 2, 4)
    uint8_t width; // Width, in tiles
    uint8_t height; // Height, in tiles
    void __wf_iram* start; // Starting address
} bitmap_t;

#define BITMAP(start, width, height, bpp) ((bitmap_t) { \
        0, \
        ((height) * ((bpp) == 4 ? 4 : 2) * 8), \
        (bpp) == 4 ? 4 : 2, \
        (bpp) == 1 ? 4 : 3, \
        (bpp), (width), (height), (start) \
    })

#define BITMAP_COLOR_MODE_STORE    0x000
#define BITMAP_COLOR_MODE_XOR      0x100
#define BITMAP_COLOR(value, mask, mode) ((value) | ((mask) << 4) | (mode)) 

uint32_t bitmap_c2p_4bpp_pixel(uint32_t pixel);

void bitmap_clear(const bitmap_t *bitmap);
void bitmap_rect_draw(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, bool rounded);
void bitmap_rect_fill(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
static inline void bitmap_rect_clear(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    bitmap_rect_fill(bitmap, x, y, width, height, BITMAP_COLOR(0, 15, BITMAP_COLOR_MODE_STORE));
}
static inline void bitmap_hline(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    bitmap_rect_fill(bitmap, x, y, length, 1, color);
}
void bitmap_vline(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void bitmap_draw_glyph(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint16_t w, uint16_t h, uint16_t bitofs, uint16_t layer, const uint8_t __far* font_data);

extern const uint16_t __far font8_bitmap[];
extern const uint16_t __far font16_bitmap[];

void bitmapfont_set_active_font(const uint16_t __far *font);
uint16_t bitmapfont_get_font_height(void);
uint16_t bitmapfont_get_char_width(uint32_t ch);
uint16_t bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint32_t ch);
uint16_t bitmapfont_get_string_width(const char __far* str, uint16_t max_width);
uint16_t bitmapfont_draw_string(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t max_width);
void bitmapfont_get_string_box(const char __far* str, uint16_t *width, uint16_t *height, int linegap);
uint16_t bitmapfont_draw_string_box(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t width, int linegap);

#endif /* __BITMAP_H__ */
