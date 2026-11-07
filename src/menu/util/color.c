/*
 * Copyright (c) 2022, 2023, 2024, 2026 Adrian Siekierka
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

#include "color.h"

uint16_t color_rgb4_to_greyscale(uint16_t rgb12) {
    return (((rgb12 >> 8) & 0xF) * 77 + ((rgb12 >> 4) & 0xF) * 150 + (rgb12 & 0xF) * 29) >> 8;
}

uint16_t color_rgb8_to_greyscale(uint8_t r, uint8_t g, uint8_t b) {
    return (77 * r + 150 * g + 29 * b) >> 8;
}

uint16_t color_rgb8_mix(uint8_t ar, uint8_t ag, uint8_t ab, uint8_t br, uint8_t bg, uint8_t bb, uint16_t ratio) {
    uint16_t iratio = 256 - ratio;

    return ((((ar * iratio) + (br * ratio)) >> 12) << 8)
        | ((((ag * iratio) + (bg * ratio)) >> 12) << 4)
        | ((((ab * iratio) + (bb * ratio)) >> 12));
}

uint16_t color_rgb4_mix(uint16_t a, uint16_t b, uint16_t ratio) {
    return color_rgb8_mix(
        ((a >> 8) & 0xF) * 0x11,
        ((a >> 4) & 0xF) * 0x11,
        (a & 0xF) * 0x11,
        ((b >> 8) & 0xF) * 0x11,
        ((b >> 4) & 0xF) * 0x11,
        (b & 0xF) * 0x11,
        ratio);
}
