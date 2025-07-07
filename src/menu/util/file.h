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

#ifndef _FILE_H_
#define _FILE_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include <nilefs.h>

// currently defined in launch.c
extern uint8_t sector_buffer[2048];

FRESULT f_open_far(FIL* fp, const char __far* path, uint8_t mode);
int16_t f_read_sram_banked(FIL* fp, uint16_t bank, uint32_t btr, uint32_t *br);
int16_t f_write_rom_banked(FIL* fp, uint16_t bank, uint32_t btw, uint32_t *bw);
int16_t f_write_sram_banked(FIL* fp, uint16_t bank, uint32_t btw, uint32_t *bw);

#endif /* _FILE_H_ */
