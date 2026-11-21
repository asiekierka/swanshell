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

#ifndef UTIL_FILE_H_
#define UTIL_FILE_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>
#include "config.h"

// currently defined in launch.c
extern uint8_t sector_buffer[CONFIG_MEMLAYOUT_SECTOR_BUFFER_SIZE];

#ifdef CONFIG_DEBUG_FORCE_DISABLE_SECTOR_BUFFER
#define sector_buffer_is_active() false
#else
#define sector_buffer_is_active ws_system_is_color_active
#endif

typedef bool (*filinfo_predicate_t)(const FILINFO __far*, const char __far*);

/**
 * Return true if any file inside the directory matches the predicate.
 */
bool f_matchdir(filinfo_predicate_t predicate, const char __far* path);
/**
 * Return true if a file at the given path exists.
 * Variant of f_exists with support for far pointers.
 */
bool f_exists_far(const char __far* path);
/**
 * Variant of f_open with support for far pointers.
 */
FRESULT f_open_far(FIL* fp, const char __far* path, uint8_t mode);
/**
 * Variant of f_unlink with support for far pointers.
 */
FRESULT f_unlink_far(const char __far* path);

int f_read_unbanked(void __far *dest, const char __far *path, uint16_t max_size, uint16_t *length);

typedef void (*fbanked_progress_callback_t)(void *userdata, uint32_t step, uint32_t max);

int16_t f_read_sram_banked(FIL* fp, uint16_t bank, uint32_t btr, fbanked_progress_callback_t cb, void *userdata);
static inline int16_t f_read_rom_banked(FIL* fp, uint16_t bank, uint32_t btr, fbanked_progress_callback_t cb, void *userdata) {
    ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
        return f_read_sram_banked(fp, bank, btr, cb, userdata);
    });
}

int16_t f_write_rom_banked(FIL* fp, uint16_t bank, uint32_t btw, fbanked_progress_callback_t cb, void *userdata, bool verify);
int16_t f_write_sram_banked(FIL* fp, uint16_t bank, uint32_t btw, fbanked_progress_callback_t cb, void *userdata, bool verify);

/**
 * Move file to a target directory.
 * @param directory Target directory.
 * @param filename Source filename.
 */
int16_t f_move_to_directory(const char __far *directory, const char __far *filename);

#endif /* UTIL_FILE_H_ */
