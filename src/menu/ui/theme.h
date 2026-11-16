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

#define THEME_BAR_STYLE_SOLID 0x0
#define THEME_BAR_STYLE_TRANSPARENT 0x1
#define THEME_BAR_STYLE_GRADIENT_LR 0x2
#define THEME_BAR_STYLE_GRADIENT_UD 0x3
#define THEME_BAR_STYLE_SHIFT 12

typedef struct {
    union {
        uint16_t background_col_a;
        struct {
            uint8_t background_col_a_low;
            uint8_t background_col_a_high;
        };
    };
    uint16_t background_col_b;
    uint16_t foreground_color;
} theme_bar_t;

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint16_t features;
    uint16_t background_color;
    uint16_t foreground_color;
    theme_bar_t tbar, bbar;

    uint8_t pad[512 - 22];
} theme_header_t;

_Static_assert(sizeof(theme_header_t) == 512, "Invalid theme header size");

/**
 * Check if the theme's current configuration contians the specified feature.
 */
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

/**
 * Reset the theme colors of the specified bar.
 */
void theme_reset_bar(theme_bar_t __far *bar);
/**
 * Reset the full theme configuration.
 */
void theme_reset(void);

/**
 * Get the bank in which theme data is stored.
 */
uint8_t theme_get_bank(void);
/**
 * Check if the theme data has been initialized.
 */
static inline bool theme_initialized(void) { return theme_get_bank() != 0xFF; }
/**
 * Initialize the theme data by loading it from the TF storage card.
 * Filesystem should be initialized first.
 */
int theme_init(void);
/**
 * Get the far memory pointer to a given theme feature, such as a wallpaper.
 *
 * @param segment WS_SRAM_SEGMENT, WS_ROM0_SEGMENT, WS_ROM1_SEGMENT
 */
void __far *theme_get_pointer(uint16_t feature, uint16_t segment);
/**
 * Save theme data to TF storage card.
 */
int theme_save(void);

#endif /* THEME_H_ */
