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
#include "util/utf8.h"

#define BITMAP_AT(bitmap, x, y) (((uint8_t*) (bitmap)->start) + ((y) * (bitmap)->y_pitch) + (((x) >> (bitmap)->x_shift) * (bitmap)->x_pitch))

#define FONT_BITMAP_SHIFT 8
#define FONT_BITMAP_SIZE 256
extern const uint16_t __far font8_bitmap[];
extern const uint16_t __far font16_bitmap[];
const uint16_t __far *font_bitmap = font16_bitmap;

static const uint16_t __far count_width_mask[17] = {
    0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

static const uint16_t __far color_mask[4] = {
    0x0000,
    0x00FF,
    0xFF00,
    0xFFFF
};

void bitmap_clear(const bitmap_t *bitmap) {
    memset(bitmap->start, 0, bitmap->x_pitch * bitmap->width);
}

extern void __bitmap_bitop_row_c(uint16_t _and, uint16_t _xor, uint16_t _rows, uint16_t _mask, bitmap_t *bitmap, void *dest);
extern void __bitmap_bitop_fill_c(uint16_t value, void *dest, uint16_t _rows);

void bitmap_vline(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    if (bitmap->bpp == 1) {
        color = BITMAP_COLOR(color ? 3 : 0, 3, BITMAP_COLOR_MODE_STORE);
    }

    uint8_t *tile = BITMAP_AT(bitmap, x, y);
    uint16_t color0 = color_mask[color & 3];
    uint16_t cmask0 = color_mask[(color >> 4) & 3];
    uint16_t color1 = color_mask[(color >> 2) & 3];
    uint16_t cmask1 = color_mask[(color >> 6) & 3];
    uint16_t cxor = (color & 0x100) ? 0xFFFF : 0x0000;

    uint16_t mask = (bitmap->bpp == 1 ? 0x1 : 0x101) << ((x & 7) ^ 7);
    __bitmap_bitop_row_c(cxor, color0, length, cmask0 & mask, bitmap, tile);
    if (bitmap->bpp == 4 && cmask1)
        __bitmap_bitop_row_c(cxor, color1, length, cmask1 & mask, bitmap, tile + 2);
}

void bitmap_rect_draw(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, bool rounded) {
    if (rounded) {
        bitmap_hline(bitmap, x + 1, y, width - 2, color);
        bitmap_hline(bitmap, x + 1, y + height - 1, width - 2, color);
    } else {
        bitmap_hline(bitmap, x, y, width, color);
        bitmap_hline(bitmap, x, y + height - 1, width, color);
    }
    bitmap_vline(bitmap, x, y + 1, height - 2, color);
    bitmap_vline(bitmap, x + width - 1, y + 1, height - 2, color);
}

void bitmap_rect_fill(bitmap_t *bitmap, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (bitmap->bpp == 1) {
        color = BITMAP_COLOR(color ? 3 : 0, 3, BITMAP_COLOR_MODE_STORE);
    }

    uint8_t row_width = 1 << bitmap->x_shift;
    uint8_t row_mask = row_width - 1;
    uint8_t *tile = BITMAP_AT(bitmap, x, y);
    uint16_t color0 = color_mask[color & 3];
    uint16_t cmask0 = color_mask[(color >> 4) & 3];
    uint16_t color1 = color_mask[(color >> 2) & 3];
    uint16_t cmask1 = color_mask[(color >> 6) & 3];
    uint16_t cxor = (color & 0x100) ? 0xFFFF : 0x0000;
    uint16_t row_mul = bitmap->bpp == 1 ? 0x1 : 0x101;

    bitmap->current_pitch = bitmap->y_pitch;

    // left border
    if (x & row_mask) {
        uint16_t next_x = (x + row_mask) & (~row_mask);
        uint16_t left_width = next_x - x;
        if (left_width > width) {
            left_width = width;
        }
        uint8_t left_offset = row_width - (x & row_mask) - left_width;

        uint8_t *vtile = tile;
        uint16_t mask = ((count_width_mask[left_width] << left_offset) * row_mul);
        // mask = (cmask0 & mask)
        // r = (r & ~mask) | (r | color0) & mask
        __bitmap_bitop_row_c(cxor, color0, height, cmask0 & mask, bitmap, vtile);
        if (bitmap->bpp == 4 && cmask1)
            __bitmap_bitop_row_c(cxor, color1, height, cmask1 & mask, bitmap, vtile + 2);

        tile += bitmap->x_pitch;
        x += left_width;
        width -= left_width;
    }

    // center
    if (cxor == 0 && cmask0 == 0xFFFF && (bitmap->bpp <= 2 || (color0 == color1 && cmask1 == 0xFFFF))) {
        while (width >= row_width) {
            __bitmap_bitop_fill_c(color0, tile, height * (bitmap->bpp >= 4 ? 2 : 1));
            tile += bitmap->x_pitch;
            x += row_width;
            width -= row_width;  
        }
    } else {
        while (width >= row_width) {
            __bitmap_bitop_row_c(cxor, color0, height, cmask0, bitmap, tile);
            if (bitmap->bpp == 4 && cmask1)
                __bitmap_bitop_row_c(cxor, color1, height, cmask1, bitmap, tile + 2);
            tile += bitmap->x_pitch;
            x += row_width;
            width -= row_width;  
        }
    }

    // right border
    if (width) {
        uint8_t *vtile = tile;
        uint16_t mask = ((count_width_mask[width] << (row_width - width)) * row_mul);
        __bitmap_bitop_row_c(cxor, color0, height, cmask0 & mask, bitmap, vtile);
        if (bitmap->bpp == 4 && cmask1)
            __bitmap_bitop_row_c(cxor, color1, height, cmask1 & mask, bitmap, vtile + 2);
    }
}

#define PER_CHAR_GAP 1

void bitmapfont_set_active_font(const uint16_t __far *font) {
    font_bitmap = font;
}

static const uint16_t __far* bitmapfont_find_char(uint32_t ch) {
    if (ch > 0xFFFFFF)
        return NULL;
    uint16_t ch_high = ch >> 8;
    if (ch_high >= FONT_BITMAP_SIZE)
        return NULL;

    uint8_t ch_low = ch & 0xFF;
    const uint8_t __far* base = MK_FP(font_bitmap[ch_high], 2);
    size_t size = 5;
    uint16_t nmemb = *((const uint16_t __far*) MK_FP(font_bitmap[ch_high], 0));
    if (nmemb == 0xFFFF) {
        return (const uint16_t __far*) (base + (ch_low << 2));
    }

    const uint8_t __far* pivot;
    size_t corr;

    while (nmemb) {
        /* algorithm needs -1 correction if remaining elements are an even number. */
        corr = nmemb & 1;
        nmemb >>= 1;
        pivot = (const uint8_t __far*) ((( const char __far* )base) + ( nmemb * size ));
        if (*pivot < ch_low) {
            base = (const uint8_t __far*) ((( const char __far* )pivot) + size);
            /* applying correction */
            nmemb -= ( 1 - corr );
        } else if (*pivot == ch_low) {
            return (const uint16_t __far*) (pivot + 1);
        }
    }

    return NULL;
}

uint16_t bitmapfont_get_char_width(uint32_t ch) {
    const uint8_t __far* data = (const uint8_t __far*) bitmapfont_find_char(ch);
    if (data == NULL) return 0;
    return (data[2] & 0xF) + (data[3] & 0xF);
}

uint16_t bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint32_t ch) {
    const uint16_t __far* data = bitmapfont_find_char(ch);
    if (data == NULL) return 0;

    uint16_t x = data[1] & 0xF;
    uint16_t y = (data[1] >>  4) & 0xF;
    uint16_t w = (data[1] >>  8) & 0xF;
    uint16_t h = (data[1] >> 12) & 0xF;
    if (!h) return x + w;

    const uint8_t __far* font_data = ((const uint8_t __far*) data) + data[0];
    xofs += x;
    yofs += y;
    uint8_t *tile = BITMAP_AT(bitmap, xofs, yofs);
    xofs &= 7;

    uint16_t pixel_fifo = *((const uint16_t __far*) font_data);
    uint16_t pixel_fifo_left = 16;
    font_data += 2;

    for (uint16_t iy = 0; iy < h; iy++, tile += bitmap->bpp) {
        uint16_t local_fifo = pixel_fifo;
        if (pixel_fifo_left < w) {
            local_fifo |= (*font_data << pixel_fifo_left);
        }

        uint8_t *dst = tile;
        int16_t px_total_left = w;
        uint16_t px_row_left = 8 - xofs;
        uint16_t px_row_offset = w >= px_row_left ? 0 : (px_row_left - w);
        if (px_row_left > px_total_left) px_row_left = px_total_left;
        
        while (px_total_left > 0) {
            uint16_t row = (local_fifo >> (px_total_left - px_row_left));
            uint8_t mask = count_width_mask[px_row_left] << px_row_offset;
            uint8_t src = (row << px_row_offset) & mask;
            *dst = (*dst & (~mask)) | src;

            px_total_left -= px_row_left;
            dst += bitmap->x_pitch;

            px_row_left = 8;
            if (px_row_left > px_total_left) px_row_left = px_total_left;
            px_row_offset = 8 - px_row_left;
        }

        if (pixel_fifo_left >= w) {
            pixel_fifo >>= w;
            pixel_fifo_left -= w;
        } else {
            pixel_fifo = (*(font_data++) >> (w - pixel_fifo_left));
            pixel_fifo_left = 8 - (w - pixel_fifo_left);
        }
        while (pixel_fifo_left <= 8) {
            pixel_fifo |= (*(font_data++) << pixel_fifo_left);
            pixel_fifo_left += 8;
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
