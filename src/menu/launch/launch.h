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

#ifndef _LAUNCH_H_
#define _LAUNCH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>

typedef struct __attribute__((packed)) {
    uint8_t jump_command;
    uint16_t jump_offset;
    uint16_t jump_segment;
    uint8_t maintenance;
    uint8_t publisher_id;
    uint8_t color;
    uint8_t game_id;
    uint8_t game_version;
    uint8_t rom_size;
    uint8_t save_type;
    uint8_t flags;
    uint8_t mapper;
    uint16_t checksum;
} rom_footer_t;

typedef struct {
    uint32_t id;
    rom_footer_t footer;
    uint16_t rom_banks;
    uint32_t sram_size;
    uint16_t eeprom_size;
    uint32_t flash_size;
    bool freya_found;
} launch_rom_metadata_t;

int16_t launch_get_rom_metadata(const char *path, launch_rom_metadata_t *meta);
int16_t launch_backup_save_data(void);
int16_t launch_restore_save_data(char *path, const launch_rom_metadata_t *meta);
int16_t launch_rom_via_bootstub(const char *path, const launch_rom_metadata_t *meta);

int16_t launch_bfb(const char *path);

#endif /* _LAUNCH_H_ */
