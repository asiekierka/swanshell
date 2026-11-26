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

#ifndef BITMAP_INTERNAL_H_
#define BITMAP_INTERNAL_H_

#include "bitmap.h"

#define BITMAP_AT(bitmap, x, y) (((uint8_t*) (bitmap)->start) + ((y) * (bitmap)->y_pitch) + (((x) >> (bitmap)->x_shift) * (bitmap)->x_pitch))

#define BITMAP_DO_ROTATE_XYWH(x,y,width,height) \
    if (!bitmap_rotation) { \
        uint16_t tmp = y; \
        y = x; \
        x = (bitmap->width << 3) - height - tmp; \
        \
        tmp = width; \
        width = height; \
        height = tmp; \
    }

#define BITMAP_DO_ROTATE_XYWH_TILES(x,y,width,height) \
    if (!bitmap_rotation) { \
        uint16_t tmp = y; \
        y = x; \
        x = bitmap->width - height - tmp; \
        \
        tmp = width; \
        width = height; \
        height = tmp; \
    }

extern const uint16_t __far count_width_mask[17];
extern const uint8_t count_width_mask8[9];
extern const uint16_t __far color_mask[4];

#endif
