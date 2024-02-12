/**
 * Copyright (c) 2024 Adrian Siekierka
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

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>

// Expected layout:
//
// AE ; width = 2
// BF ; height = 4
// CG ; bpp = 2 
// DH ; pitch = 64
typedef struct {
    uint16_t pitch; // Pitch, in bytes
    void __wf_iram* start; // Starting address
    uint8_t width; // Width, in tiles
    uint8_t height; // Height, in tiles
    uint8_t bpp; // Bits per pixel
} bitmap_t;

#define BITMAP(start, width, height, bpp) ((bitmap_t) { \
        ((height) * (bpp) * 8), \
        (start), (width), (height), (bpp) \
    })

void bitmap_clear(const bitmap_t *bitmap);
void bitmap_rect_clear(const bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
uint16_t bitmapfont_get_char_width(uint32_t ch);
uint16_t bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint32_t ch);
uint16_t bitmapfont_get_string_width(const char __far* str, uint16_t max_width);
uint16_t bitmapfont_draw_string(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t max_width);

#endif /* __BITMAP_H__ */
