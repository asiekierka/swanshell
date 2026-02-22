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

#include <string.h>
#include <ws.h>
#include <ws/system.h>
#include <wsx/utf8.h>
#include "bitmap.h"
#include "config.h"
#include "util/math.h"

#define BITMAP_AT(bitmap, x, y) (((uint8_t*) (bitmap)->start) + ((y) * (bitmap)->y_pitch) + (((x) >> (bitmap)->x_shift) * (bitmap)->x_pitch))

static const uint16_t __far count_width_mask[17] = {
    0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

// This is not __far to optimize glyph drawing. -9 bytes RAM
static const uint8_t count_width_mask8[9] = {
    0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF
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
        color = BITMAP_COLOR_2BPP(color ? 3 : 0);
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
        color = BITMAP_COLOR_2BPP(color ? 3 : 0);
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
        if (bitmap->bpp == 4 && cmask1) {
            while (width >= row_width) {
                __bitmap_bitop_row_c(cxor, color0, height, cmask0, bitmap, tile);
                __bitmap_bitop_row_c(cxor, color1, height, cmask1, bitmap, tile + 2);
                tile += bitmap->x_pitch;
                x += row_width;
                width -= row_width;  
            }
        } else {
            while (width >= row_width) {
                __bitmap_bitop_row_c(cxor, color0, height, cmask0, bitmap, tile);
                tile += bitmap->x_pitch;
                x += row_width;
                width -= row_width;  
            }
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

void bitmap_draw_glyph(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint16_t w, uint16_t h, uint16_t layer, const uint8_t __far* font_data) {
    if (!FP_SEG(font_data)) return;

    xofs += w - 1;
    uint8_t *tile = BITMAP_AT(bitmap, xofs, yofs) + layer;
    xofs &= 7;

    uint16_t pixel_fifo = *((const uint16_t __far*) font_data);
    uint16_t pixel_fifo_left = 16;
    font_data += 2;

    uint8_t *tile_end = tile + (bitmap->bpp * h);
    uint16_t px_row_offset = xofs ^ 7;
    uint16_t px_row_left_first = MIN(8 - px_row_offset, w);

    while (tile < tile_end) {
        uint8_t *dst = tile;
        int16_t px_total_left = w;
        uint16_t px_row_left = px_row_left_first;

        if (pixel_fifo_left < px_row_left) {
            pixel_fifo |= (*(font_data++) << pixel_fifo_left);
            pixel_fifo_left += 8;
        }

        uint8_t mask = count_width_mask8[px_row_left] << px_row_offset;
        uint8_t src = (pixel_fifo << px_row_offset) & mask;
        *dst = (*dst & (~mask)) | src;

        px_total_left -= px_row_left;
        dst -= bitmap->x_pitch;
        pixel_fifo >>= px_row_left;
        pixel_fifo_left -= px_row_left;

        while (px_total_left > 0) {
            uint16_t px_row_left = MIN(8, px_total_left);

            if (pixel_fifo_left < px_row_left) {
                pixel_fifo |= (*(font_data++) << pixel_fifo_left);
                pixel_fifo_left += 8;
            }

            uint8_t mask = count_width_mask8[px_row_left];
            uint8_t src = pixel_fifo & mask;
            *dst = (*dst & (~mask)) | src;

            px_total_left -= px_row_left;
            dst -= bitmap->x_pitch;
            pixel_fifo >>= px_row_left;
            pixel_fifo_left -= px_row_left;
        }
        
        tile += bitmap->bpp;
    }
}

void bitmap_vscroll_row(const bitmap_t *bitmap, uint16_t ix, uint16_t row_from, uint16_t row_to, uint16_t height) {
    if (ws_system_is_color_active()) {
        uint16_t dst = (uint16_t) BITMAP_AT(bitmap, ix << 3, row_to);
        uint16_t src = (uint16_t) BITMAP_AT(bitmap, ix << 3, row_from); 
        uint8_t mode = WS_GDMA_CTRL_INC | WS_GDMA_CTRL_START;
        if (row_from < row_to) {
            src += (bitmap->bpp * height) - 2;
            dst += (bitmap->bpp * height) - 2;
            mode |= WS_GDMA_CTRL_DEC;
        }
        outportw(WS_GDMA_LENGTH_PORT, (bitmap->bpp * height));
        outportw(WS_GDMA_SOURCE_L_PORT, src);
        outportw(WS_GDMA_DEST_PORT, dst);
        outportb(WS_GDMA_SOURCE_H_PORT, 0);
        outportb(WS_GDMA_CTRL_PORT, mode);
    } else {
        _nmemmove(BITMAP_AT(bitmap, ix << 3, row_to), BITMAP_AT(bitmap, ix << 3, row_from), bitmap->bpp * height);
    }
}