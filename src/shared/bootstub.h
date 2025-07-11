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

#ifndef __BOOTSTUB_H__
#define __BOOTSTUB_H__

#include <stddef.h>
#include <stdint.h>

#define BOOTSTUB_PROG_PATCH_FREYA_SOFT_RESET 0x01

typedef struct {
    // FAT information
    uint8_t fs_type;
    uint32_t cluster_table_base;
    uint32_t data_base;
    uint16_t cluster_size;
    uint32_t fat_entry_count;

    // Program information
    uint32_t prog_size; ///< Program size
    uint16_t rom_banks; ///< Requested ROM banks
    uint32_t prog_cluster; ///< Starting cluster for program
    uint8_t prog_sram_mask;
    uint8_t prog_pow_cnt;
    uint8_t prog_emu_cnt;
    uint8_t prog_flags; ///< IO_SYSTEM_CTRL1 flags
    uint8_t prog_flags2; ///< IO_SYSTEM_CTRL2 flags
    uint8_t prog_patches;
} bootstub_data_t;

#define bootstub_data ((volatile bootstub_data_t*) 0x0060)

#endif
