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

#include <string.h>
#include "bitmap.h"
#include "../util/utf8.h"
#include "../../../build/menu/assets/menu/fonts.h"

#define BITMAP_AT(bitmap, x, y) (((uint8_t*) (bitmap)->start) + ((y) * (bitmap)->bpp) + (((x) >> 3) * (bitmap)->pitch))

static const uint8_t __far count_width_mask[9] = {
    0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF
};

void bitmap_clear(const bitmap_t *bitmap) {
    memset(bitmap->start, 0, bitmap->pitch * bitmap->width);
}

void bitmap_rect_clear(const bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint8_t *tile = BITMAP_AT(bitmap, x, y);

    // left border
    if (x & 7) {
        uint16_t next_x = (x + 7) & (~7);
        uint16_t left_width = next_x - x;
        if (left_width > width) {
            left_width = width;
        }
        uint8_t left_offset = 8 - (x & 7) - left_width;

        uint8_t *vtile = tile;
        uint8_t mask = ~(count_width_mask[left_width] << left_offset);
        for (uint16_t i = 0; i < height * bitmap->bpp; i++) {
            *(vtile++) &= mask;
        }

        x += left_width;
        width -= left_width;
    }

    // center
    while (width >= 8) {
        memset(tile, 0, height * bitmap->bpp);
        tile += bitmap->pitch;
        x += 8;
        width -= 8;    
    }

    // right border
    if (width) {
        uint8_t *vtile = tile;
        uint8_t mask = ~(count_width_mask[width] << (7 - width));
        for (uint16_t i = 0; i < height * bitmap->bpp; i++) {
            *(vtile++) &= mask;
        }
    }
}

// font header format:
// - word 0: character (0-65535)
// - word 1: ROM offset (0-65535)
// - word 2:
//   - bits 0-3: x
//   - bits 4-7: y
//   - bits 8-11: width
//   - bits 12-15: height

#define PER_CHAR_GAP 1

static const uint16_t __far* bitmapfont_find_char(uint32_t ch) {
    if (ch >= 0x10000)
        return NULL;

    const uint16_t __far* base = (const uint16_t __far*) font_table;
    size_t size = 6;
    uint16_t nmemb = font_table_size / size;

    const uint16_t __far* pivot;
    size_t corr;

    while (nmemb) {
        /* algorithm needs -1 correction if remaining elements are an even number. */
        corr = nmemb & 1;
        nmemb >>= 1;
        pivot = (const uint16_t __far*) ((( const char __far* )base) + ( nmemb * size ));
        if (*pivot < ch) {
            base = (const uint16_t __far*) ((( const char __far* )pivot) + size);
            /* applying correction */
            nmemb -= ( 1 - corr );
        } else if (*pivot == ch) {
            return pivot;
        }
    }

    return NULL;
}

uint16_t bitmapfont_get_char_width(uint32_t ch) {
    const uint8_t __far* data = (const uint8_t __far*) bitmapfont_find_char(ch);
    if (data == NULL) return 0;
    return (data[4] & 0xF) + (data[5] & 0xF);
}

uint16_t bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint32_t ch) {
    const uint16_t __far* data = bitmapfont_find_char(ch);
    if (data == NULL) return 0;

    uint16_t x = data[2] & 0xF;
    uint16_t y = (data[2] >>  4) & 0xF;
    uint16_t w = (data[2] >>  8) & 0xF;
    uint16_t h = (data[2] >> 12) & 0xF;
    // rom_offset not shifted
    // const uint8_t __far* font_data = ((const uint8_t __far*) font_bitmap) + data[1];
    // rom_offset shifted
    const uint8_t __far* font_data = MK_FP(FP_SEG(font_bitmap) + ((data[1] >> 3) & 0x1FFF), FP_OFF(font_bitmap) + ((data[1] << 1) & 0x000E));
    xofs += x;
    yofs += y;
    uint8_t *tile = BITMAP_AT(bitmap, xofs, yofs);
    xofs &= 7;

    uint16_t pixels = *((const uint16_t __far*) font_data);
    uint16_t pixels_left = 16;
    font_data += 2;

    for (uint16_t iy = 0; iy < h; iy++, tile += bitmap->bpp) {
        uint8_t *todrawtile = tile;
        int16_t todraw = w;
        uint16_t candraw = 8 - xofs;
        uint16_t drawofs = w >= candraw ? 0 : (candraw - w);
        while (todraw > 0) {
            uint8_t bw = (candraw > w ? w : candraw);
            uint16_t row = (pixels >> (todraw - bw));
            uint8_t mask = count_width_mask[bw] << drawofs;
            uint8_t rowe = (row << drawofs) & mask;
            *todrawtile = (*todrawtile & (~mask)) | rowe;

            todraw -= candraw;
            todrawtile += bitmap->pitch;

            candraw = todraw;
            drawofs = 8 - candraw;
        }

        pixels >>= w;
        pixels_left -= w;
        if (pixels_left <= 8) {
            pixels |= (*(font_data++) << pixels_left);
            pixels_left += 8;
        }
    }

    return x + w;
}

uint16_t bitmapfont_get_string_width(const char __far* str, uint16_t max_width) {
    uint32_t ch;
    uint16_t width = 0;
    while ((ch = utf8_decode_char(&str)) != 0) {
        width += bitmapfont_get_char_width(ch) + PER_CHAR_GAP;
        if (width - PER_CHAR_GAP >= max_width)
            return width - PER_CHAR_GAP;
    }
    return width;
}

uint16_t bitmapfont_draw_string(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t max_width) {
    uint32_t ch;
    uint16_t width = 0;

    while ((ch = utf8_decode_char(&str)) != 0) {
        uint16_t w = bitmapfont_draw_char(bitmap, xofs, yofs, ch) + PER_CHAR_GAP;
        xofs += w;
        width += w;
        if (width - PER_CHAR_GAP >= max_width)
            return width - PER_CHAR_GAP;
    }
    return width;
}
