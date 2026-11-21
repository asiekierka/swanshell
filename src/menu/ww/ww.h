/**
 * Copyright (c) 2025 Adrian Siekierka
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

#ifndef WW_WW_H_
#define WW_WW_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

int16_t ww_ui_extract_from_rom(const char __far* filename);
int16_t ww_ui_replace_component(const char __far* filename, bool is_os);
int16_t ww_ui_create_image(void);
int16_t ww_ui_move_to_fbin(const char __far *filename);

static inline uint32_t ww_fat_to_freya_time(uint16_t fdate, uint16_t ftime) {
    return ((((uint32_t) fdate) << 16) + (20 << 9)) | ftime;
}

static inline uint16_t ww_freya_to_fat_date(uint32_t mtime) {
    return (mtime >> 16) - (20 << 9);
}

static inline uint16_t ww_freya_to_fat_time(uint32_t mtime) {
    return (uint16_t) mtime;
}

#endif /* WW_WW_H_ */
