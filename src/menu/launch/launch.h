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

#ifndef LAUNCH_H_
#define LAUNCH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>
#include "bootstub.h"

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
    uint8_t rom_type;
} launch_rom_metadata_t;

bool launch_is_battery_required(launch_rom_metadata_t *meta);
int16_t launch_get_rom_metadata_psram(launch_rom_metadata_t *meta);
int16_t launch_get_rom_metadata(const char *path, launch_rom_metadata_t *meta);
int16_t launch_backup_save_data(void);
int16_t launch_restore_save_data(char *path, const launch_rom_metadata_t *meta);
bool launch_ui_handle_battery_missing_error(launch_rom_metadata_t *meta);
bool launch_ui_handle_mcu_comm_error(launch_rom_metadata_t *meta);
int16_t launch_set_bootstub_file_entry(const char *path, bootstub_file_entry_t *entry);
int16_t launch_rom_via_bootstub(const launch_rom_metadata_t *meta);

int16_t launch_bfb(const char *path);
int16_t launch_bfb_in_psram(void);

int16_t launch_com(const char *path);

int16_t launch_in_psram(uint32_t size);

#endif /* LAUNCH_H_ */
