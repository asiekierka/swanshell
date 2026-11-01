/**
 * Copyright (c) 2023, 2024, 2025, 2026 Adrian Siekierka
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

#include "bmp.h"
#include <ws.h>
#include <wsx/planar_convert.h>
#include "errors.h"
#include "util/math.h"

int bmp_calculate_color_count(const bmp_header_t __far *bmp, const void __far *data) {
    int color_count = bmp->color_count;
    if (!color_count) {
        color_count = 1 << bmp->bpp;
    }
    if (bmp->bpp == 8 && color_count > 16) {
        color_count = 1;
        const uint8_t __far *data_chk = data;
        for (uint16_t y = 0; y < bmp->height; y++) {
            for (uint16_t x = 0; x < bmp->width; x++, data_chk++) {
                color_count = MAX(color_count, *data_chk + 1);
            }
        }
    }
    return color_count;
}

int bmp_convert_palette_to_rgb4(uint16_t *dest, const void __far *src, int color_count, uint16_t flags) {
    const uint8_t __far *palette = src;
    for (int i = 0; i < color_count; i++) {
        uint16_t b = *(palette++);
        uint16_t g = *(palette++);
        uint16_t r = *(palette++);
        palette++;
        *(dest++) = WS_RGB(r >> 4, g >> 4, b >> 4);
    }
    return 0;
}

int bmp_convert_data(uint16_t width, uint16_t height, uint16_t pitch, uint16_t bpp, void *dest, const void __far *src, uint16_t flags) {
    const uint8_t __far *src8 = src;
    if (bpp == 8) {
        if (!(flags & BMP_LOADER_FLAG_4BPP)) return ERR_FILE_FORMAT_INVALID;
        for (uint8_t y = 0; y < height; y++, src8 += pitch) {
            uint16_t __far *line_src = (uint16_t __far*) src8;
            uint8_t *line_dst = (uint8_t*) (((uintptr_t) dest) + ((height - 1 - y) * 4));
            for (uint8_t x = 0; x < width; x += 8, line_dst += WS_DISPLAY_HEIGHT_TILES * 32 - 4) {
                uint16_t s;
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
            }
        }
    } else if (bpp == 4) {
        if (!(flags & BMP_LOADER_FLAG_4BPP)) return ERR_FILE_FORMAT_INVALID;
        for (uint8_t y = 0; y < height; y++, src8 += pitch) {
            uint32_t __far *line_src = (uint32_t __far*) src8;
            uint32_t *line_dst = (uint32_t*) (((uintptr_t) dest) + ((height - 1 - y) * 4));
            for (uint8_t x = 0; x < width; x += 8, line_src++, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                *line_dst = *line_src;
            }
        }
    } else if (bpp == 1) {
        if (flags & BMP_LOADER_FLAG_4BPP) {
            for (uint8_t y = 0; y < height; y++, src8 += pitch) {
                uint8_t __far *line_src = (uint8_t __far*) src8;
                uint32_t *line_dst = (uint32_t*) (((uintptr_t) dest) + ((height - 1 - y) * 4));
                for (uint8_t x = 0; x < width; x += 8, line_src++, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                    *line_dst = *line_src;
                }
            }
        } else {
            for (uint8_t y = 0; y < height; y++, src8 += pitch) {
                uint8_t __far *line_src = (uint8_t __far*) src8;
                uint16_t *line_dst = (uint16_t*) (((uintptr_t) dest) + ((height - 1 - y) * 2));
                for (uint8_t x = 0; x < width; x += 8, line_src++, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                    *line_dst = *line_src;
                }
            }
        }
    } else {
        return ERR_FILE_FORMAT_INVALID;
    }
    if ((flags & (BMP_LOADER_FLAG_4BPP | BMP_LOADER_FLAG_PACKED)) == BMP_LOADER_FLAG_4BPP) {
        for (uint8_t y = 0; y < height; y++) {
            uint32_t *line_dst = ((uint32_t*) dest) + y;
            for (uint8_t x = 0; x < width; x += 8, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                *line_dst = wsx_planar_convert_4bpp_packed_row(*line_dst);
            }
        }
    }
    return 0;
}
