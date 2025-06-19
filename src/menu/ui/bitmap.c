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
#include <wsx/utf8.h>
#include "bitmap.h"

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

void bitmapfont_set_active_font(const uint16_t __far *font) {
    font_bitmap = font;
}

static inline uint16_t __bitmapfont_get_font_height(void) {
    return font_bitmap[0];
}

uint16_t bitmapfont_get_font_height(void) {
    return __bitmapfont_get_font_height();
}

static const uint16_t __far* bitmapfont_find_char(uint32_t ch) {
    if (ch > 0xFFFFFF)
        return NULL;
    uint16_t ch_high = (ch >> 8) + 1;
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

static inline uint16_t bitmapfont_get_char_width_a(const uint16_t __far* data16) {
    const uint8_t __far *data = (const uint8_t __far*) data16;
    if (data == NULL) return 0;
    return (data[2] & 0xF) + (data[3] & 0xF);    
}

uint16_t bitmapfont_get_char_width(uint32_t ch) {
    return bitmapfont_get_char_width_a(bitmapfont_find_char(ch));
}

static uint16_t bitmapfont_draw_char_a(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const uint16_t __far* data) {
    if (data == NULL) return 0;

    uint16_t x = data[1] & 0xF;
    uint16_t y = (data[1] >>  4) & 0xF;
    uint16_t w = (data[1] >>  8) & 0xF;
    uint16_t h = (data[1] >> 12) & 0xF;
    if (!h) return x + w;

    const uint8_t __far* font_data = ((const uint8_t __far*) data) + data[0];
    xofs += x + w - 1;
    yofs += y;
    uint8_t *tile = BITMAP_AT(bitmap, xofs, yofs);
    xofs &= 7;

    uint16_t pixel_fifo = *((const uint16_t __far*) font_data);
    uint16_t pixel_fifo_left = 16;
    font_data += 2;

    for (uint16_t iy = 0; iy < h; iy++, tile += bitmap->bpp) {
        uint8_t *dst = tile;
        int16_t px_total_left = w;
        uint16_t px_row_offset = xofs ^ 7;

        while (px_total_left > 0) {
            uint16_t px_row_left = 8 - px_row_offset;
            if (px_row_left > px_total_left) px_row_left = px_total_left;

            if (pixel_fifo_left < px_row_left) {
                pixel_fifo |= (*(font_data++) << pixel_fifo_left);
                pixel_fifo_left += 8;
            }

            uint8_t mask = count_width_mask[px_row_left] << px_row_offset;
            uint8_t src = (pixel_fifo << px_row_offset) & mask;
            *dst = (*dst & (~mask)) | src;

            px_total_left -= px_row_left;
            dst -= bitmap->x_pitch;
            pixel_fifo >>= px_row_left;
            pixel_fifo_left -= px_row_left;

            px_row_offset = 0;
        }
    }

    return x + w;
}

uint16_t bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint32_t ch) {
    return bitmapfont_draw_char_a(bitmap, xofs, yofs, bitmapfont_find_char(ch));
}

uint16_t bitmapfont_get_string_width(const char __far* str, uint16_t max_width) {
    uint32_t ch;
    uint16_t width = 0;
    
    while ((ch = wsx_utf8_decode_next(&str)) != 0) {
        uint16_t new_width = width + bitmapfont_get_char_width(ch);
        if (new_width > max_width)
            return width - BITMAPFONT_CHAR_GAP;

        width = new_width + BITMAPFONT_CHAR_GAP;
    }

    return width - BITMAPFONT_CHAR_GAP;
}

uint16_t bitmapfont_draw_string(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t max_width) {
    uint32_t ch;
    uint16_t width = 0;

    while ((ch = wsx_utf8_decode_next(&str)) != 0) {
        const uint16_t __far* data = bitmapfont_find_char(ch);
        uint16_t new_width = width + bitmapfont_get_char_width_a(data);
        if (new_width > max_width)
            return width - BITMAPFONT_CHAR_GAP;

        uint16_t w = bitmapfont_draw_char_a(bitmap, xofs, yofs, data) + BITMAPFONT_CHAR_GAP;
        xofs += w;
        width += w;
    }

    return width - BITMAPFONT_CHAR_GAP;
}

void bitmapfont_get_string_box(const char __far* str, uint16_t *width, uint16_t *height) {
    uint32_t ch;
    uint16_t line_width = 0;
    uint16_t max_width = 0;
    const char __far* break_str = NULL;
    uint16_t break_width = 0;

    *height = 0;
    while (true) {
        ch = wsx_utf8_decode_next(&str);
repeat_char:
        ;
        bool is_soft_break = ch == ' ';
        bool is_hard_break = ch == '\n';
        if (is_soft_break && !line_width) {
            continue;
        }
        uint16_t new_line_width = line_width + bitmapfont_get_char_width(ch);
        if (is_soft_break || is_hard_break) {
            break_str = str;
            break_width = line_width;
        }
        if (new_line_width > *width || is_hard_break || !ch) {
            bool char_consumed = false;
            if (new_line_width <= *width) {
                line_width = new_line_width;
            } else if (break_str != NULL) {
                str = break_str;
                line_width = break_width;
                break_str = NULL;
                char_consumed = true;
            }
            if (max_width < line_width) {
                max_width = line_width;
            }
            line_width = 0;
            *height += __bitmapfont_get_font_height();
            if (!ch) {
                break;
            } else if (!char_consumed) {
                goto repeat_char;
            }
        } else {
            line_width = new_line_width + BITMAPFONT_CHAR_GAP;
        }
    }

    *width = max_width;
}

uint16_t bitmapfont_draw_string_box(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t width) {
    uint32_t ch;
    uint16_t line_width = 0;
    const char __far* start_str = str;
    const char __far* prev_str = str;
    const char __far* break_str = NULL;
    uint16_t start_yofs = yofs;

    while (true) {
        ch = wsx_utf8_decode_next(&str);
        bool is_soft_break = ch == ' ';
        bool is_hard_break = ch == '\n';
        if (is_soft_break && !line_width) {
            continue;
        }
        uint16_t new_line_width = line_width + bitmapfont_get_char_width(ch);
        if (is_soft_break || is_hard_break) {
            break_str = str;
        }
        if (new_line_width > width || is_hard_break || !ch) {
            if (!ch) {
                break_str = str;
            } else if (break_str == NULL) {
                break_str = prev_str;
            }
            uint16_t local_xofs = xofs;
            while (start_str < break_str) {
                ch = wsx_utf8_decode_next(&start_str);
                local_xofs += bitmapfont_draw_char(bitmap, local_xofs, yofs, ch) + BITMAPFONT_CHAR_GAP;
            }
            str = break_str;
            start_str = break_str;
            prev_str = break_str;
            break_str = NULL;
            line_width = 0;
            yofs += __bitmapfont_get_font_height();
            if (!ch) {
                break;
            }
        } else {
            line_width = new_line_width + BITMAPFONT_CHAR_GAP;
            prev_str = str;
        }
    }
    return yofs - start_yofs;
}
