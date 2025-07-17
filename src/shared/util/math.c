/**
 * Copyright (c) 2022, 2023, 2024 Adrian Siekierka
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

#include "math.h"

uint32_t math_next_power_of_two(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

uint16_t math_color_to_greyscale(uint16_t rgb12) {
    return (((rgb12 >> 8) & 0xF) * 77 + ((rgb12 >> 4) & 0xF) * 150 + (rgb12 & 0xF) * 29) >> 8;
}

uint16_t math_popcount16(uint16_t v) {
    uint16_t result = 0;
    while (v) {
        result++;
        v &= v - 1;
    }
    return result;
}
