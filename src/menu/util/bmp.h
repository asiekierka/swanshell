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

#ifndef UTIL_BMP_H_
#define UTIL_BMP_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

typedef struct {
    uint16_t magic;
    uint32_t size;
    uint16_t res0, res1;
    uint32_t data_start;
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t colorplanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t data_size;
    uint32_t hres, vres;
    uint32_t color_count;
    uint32_t important_color_count;
} bmp_header_t;

#define BMP_MIN_HEADER_SIZE 40
// 'BM'
#define BMP_MAGIC 0x4D42

#define BMP_LOADER_FLAG_4BPP 0x0001
#define BMP_LOADER_FLAG_PACKED 0x0002

static inline bool bmp_is_header_valid(const bmp_header_t __far *bmp) {
    return !(bmp->magic != BMP_MAGIC || bmp->header_size < BMP_MIN_HEADER_SIZE ||
        bmp->width <= 0 || bmp->height <= 0);
}

static inline uint16_t bmp_calculate_pitch_bytes(const bmp_header_t __far *bmp) {
    return (((bmp->width * bmp->bpp) + 31) / 32) << 2;
}

int bmp_calculate_color_count(const bmp_header_t __far *bmp, const void __far *data);
int bmp_convert_palette_to_rgb4(uint16_t *dest, const void __far *src, int color_count, uint16_t flags);
int bmp_convert_data(uint16_t width, uint16_t height, uint16_t pitch, uint16_t bpp, void *dest, const void __far *src, uint16_t flags);

#endif /* UTIL_BMP_H_ */
