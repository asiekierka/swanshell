/**
 * Copyright (c) 2026 Adrian Siekierka
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

#ifndef THEME_H_
#define THEME_H_

#include <wonderful.h>

#define THEME_MAGIC 0x7433
#define THEME_VERSION 1

#define THEME_FEATURE_WALLPAPER_HORIZONTAL 0x0001
#define THEME_FEATURE_WALLPAPER_VERTICAL 0x0002
#define THEME_FEATURE_WALLPAPER_SHARED 0x0004

#define THEME_MEM_HEADER ((theme_header_t __far*) MK_FP(0x1000, 0x0000))

typedef struct {
    uint16_t background_color;
    uint16_t foreground_color;
} theme_bar_t;

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint16_t features;
    uint16_t accent_color;
    uint16_t background_color;
    uint16_t foreground_color;

    uint8_t pad[512 - 12];
} theme_header_t;

_Static_assert(sizeof(theme_header_t) == 512, "Invalid theme header size");

bool theme_has_feature(uint16_t feature);
/**
 * Add the specified .bmp file as a wallpaper.
 *
 * @param feature HORIZONTAL/VERTICAL for one, SHARED for both.
 * @param path Path to .bmp file.
 * @return Error code, 0 on success.
 */
int theme_add_wallpaper(uint16_t feature, const char __far *path);
/**
 * Remove specified wallpaper/s.
 *
 * @param feature HORIZONTAL/VERTICAL for one, SHARED for both
 */
void theme_remove_wallpaper(uint16_t feature);

uint8_t theme_get_bank(void);
static inline bool theme_initialized(void) { return theme_get_bank() != 0xFF; }
int theme_init(void);
void __far *theme_get_pointer(uint16_t feature);
int theme_save(void);

#endif /* THEME_H_ */
