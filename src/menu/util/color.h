/**
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

#ifndef COLOR_H_
#define COLOR_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

uint16_t color_rgb4_to_greyscale(uint16_t rgb12);
uint16_t color_rgb8_to_greyscale(uint8_t r, uint8_t g, uint8_t b);

uint16_t color_rgb4_mix(uint16_t a, uint16_t b, uint16_t ratio);
uint16_t color_rgb8_mix(uint8_t ar, uint8_t ag, uint8_t ab, uint8_t br, uint8_t bg, uint8_t bb, uint16_t ratio);

#endif /* COLOR_H_ */
