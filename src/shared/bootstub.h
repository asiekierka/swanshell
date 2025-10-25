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
#define BOOTSTUB_CLUSTER_AT_PSRAM 0xFFFFFFF8

#define ROM_TYPE_UNKNOWN 0x00
#define ROM_TYPE_WS      0x01
#define ROM_TYPE_PCV2    0x02
#define ROM_TYPE_FREYA   0x03

typedef struct {
    uint32_t cluster; ///< Initial FAT cluster
    uint32_t size; ///< File size, in bytes
} bootstub_file_entry_t;

typedef struct {
    // FAT information
    uint8_t fs_type;
    uint32_t cluster_table_base;
    uint32_t data_base;
    uint16_t cluster_size;
    uint32_t fat_entry_count;

    // Program information
    bootstub_file_entry_t prog;
    void __far* start_pointer; ///< Start pointer
    uint16_t rom_banks; ///< Requested ROM banks
    uint8_t prog_sram_mask;
    uint8_t prog_pow_cnt;
    uint8_t prog_emu_cnt;
    uint8_t prog_flags; ///< IO_SYSTEM_CTRL1 flags
    uint8_t prog_flags2; ///< IO_SYSTEM_CTRL2 flags
    uint8_t prog_patches;
    uint8_t prog_rom_type;
} bootstub_data_t;

#define bootstub_data ((volatile bootstub_data_t*) 0x0060)

#endif
