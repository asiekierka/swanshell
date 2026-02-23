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

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include <nilefs.h>

#define SETTING_TYPE_CATEGORY    0
#define SETTING_TYPE_FLAG        1
#define SETTING_TYPE_CHOICE_BYTE 2
#define SETTING_TYPE_COLOR       3
#define SETTING_TYPE_ACTION      4

#define SETTING_FILE_SHOW_HIDDEN_SHIFT 0
#define SETTING_FILE_SHOW_HIDDEN       (1 << SETTING_FILE_SHOW_HIDDEN_SHIFT)

#define SETTING_FILE_HIDE_ICONS_SHIFT 1
#define SETTING_FILE_HIDE_ICONS       (1 << SETTING_FILE_HIDE_ICONS_SHIFT)

#define SETTING_FILE_TEXT_READER_SMALL_SHIFT 2
#define SETTING_FILE_TEXT_READER_SMALL       (1 << SETTING_FILE_TEXT_READER_SMALL_SHIFT)

#define SETTING_FILE_SHOW_SAVES_SHIFT 3
#define SETTING_FILE_SHOW_SAVES       (1 << SETTING_FILE_SHOW_SAVES_SHIFT)

#define SETTING_FILE_SORT_FILESYSTEM 0
#define SETTING_FILE_SORT_NAME_ASC   1
#define SETTING_FILE_SORT_NAME_DESC  2
#define SETTING_FILE_SORT_DATE_ASC   3
#define SETTING_FILE_SORT_DATE_DESC  4
#define SETTING_FILE_SORT_SIZE_ASC   5
#define SETTING_FILE_SORT_SIZE_DESC  6

#define SETTING_FILE_VIEW_LARGE 0
#define SETTING_FILE_VIEW_SMALL 1

#define SETTING_THEME_DARK_MODE_SHIFT 4
#define SETTING_THEME_DARK_MODE       (1 << SETTING_THEME_DARK_MODE_SHIFT)

#define SETTING_FLAG_COLOR_ONLY 0x01
#define SETTING_FLAG_CHOICE_LIST 0x80
#define SETTING_FLAG_ACTION_NO_ARROW 0x80

enum {
    SETTING_MCU_SPI_SPEED_384KHZ = 0,
    SETTING_MCU_SPI_SPEED_6MHZ,
    SETTING_MCU_SPI_SPEED_24MHZ,
    SETTING_MCU_SPI_SPEED_COUNT
};

struct setting;

typedef struct setting_category {
    uint16_t name;
    uint16_t help;
    const struct setting_category __far *parent;
    uint16_t entry_count;
    const struct setting __far* entries[];
} setting_category_t;

typedef struct setting {
    const char __far* key;
    uint16_t name;
    uint16_t help;
    uint8_t type;
    uint8_t flags;
    void (*on_change)(const struct setting __far*);
    union {
        struct {
            const struct setting_category __far *value;
        } category;
        struct {
            uint8_t *value;
            uint8_t bit;
            uint16_t name_false;
            uint16_t name_true;
        } flag;
        struct {
            void *value;
            uint16_t min;
            uint16_t max;
            bool (*allowed)(uint16_t);
            void (*name)(uint16_t, char *, int);
        } choice;
        struct {
            uint16_t *value;
        } color;
    };
} setting_t;

#define SETTING_PROG_FLAG_SRAM_OVERCLOCK_SHIFT 0
#define SETTING_PROG_FLAG_SRAM_OVERCLOCK (1 << SETTING_PROG_FLAG_SRAM_OVERCLOCK_SHIFT)

#define SETTING_PROG_FLAG_FX_COMPATIBLE_BIOS_SHIFT 1
#define SETTING_PROG_FLAG_FX_COMPATIBLE_BIOS (1 << SETTING_PROG_FLAG_FX_COMPATIBLE_BIOS_SHIFT)

typedef struct __attribute__((packed)) {
    uint8_t flags;
} settings_prog_t;

#define SETTING_THEME_ACCENT_COLOR_DEFAULT 0x4A7

typedef struct __attribute__((packed)) {
    settings_prog_t prog;
    uint8_t file_flags;
    uint8_t file_sort;
    uint8_t language;
    uint8_t file_view;
    uint8_t mcu_spi_speed;
    uint8_t joy_repeat_first_ticks;
    uint8_t joy_repeat_next_ticks;
    union {
        struct {
            uint8_t accent_color_low;
            uint8_t accent_color_high;
        };
        uint16_t accent_color;
    };
} settings_t;

extern settings_t settings;
extern const setting_category_t __far settings_root;
extern const setting_t __far setting_language;

void settings_reset(void);
int16_t settings_load(void);
int16_t settings_save(void);

void settings_language_update(void);
bool settings_language_prefer_large_fonts(void);

#endif /* _SETTINGS_H_ */
