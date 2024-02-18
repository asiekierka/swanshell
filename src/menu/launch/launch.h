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

#ifndef _LAUNCH_H_
#define _LAUNCH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>

typedef struct {
    uint8_t jump_call[5];
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
    rom_footer_t footer;
    uint32_t sram_size;
    uint16_t eeprom_size;
    uint32_t flash_size;
} launch_rom_metadata_t;

uint8_t launch_get_rom_metadata(const char *path, launch_rom_metadata_t *meta);
uint8_t launch_backup_save_data(void);
uint8_t launch_restore_save_data(char *path, const launch_rom_metadata_t *meta);
uint8_t launch_rom_via_bootstub(const char *path, const launch_rom_metadata_t *meta);

#endif /* _LAUNCH_H_ */
