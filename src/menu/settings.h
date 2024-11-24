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
#define SETTING_TYPE ACTION 3

#define SETTING_FILE_SHOW_HIDDEN_SHIFT 0
#define SETTING_FILE_SHOW_HIDDEN       (1 << SETTING_FILE_SHOW_HIDDEN_SHIFT)

#define SETTING_FILE_SORT_DEFAULT   0
#define SETTING_FILE_SORT_NAME_ASC  1
#define SETTING_FILE_SORT_NAME_DESC 2
#define SETTING_FILE_SORT_DATE_ASC  3
#define SETTING_FILE_SORT_DATE_DESC 4
#define SETTING_FILE_SORT_SIZE_ASC  5
#define SETTING_FILE_SORT_SIZE_DESC 6

struct setting;

typedef struct setting_category {
    uint16_t name;
    const struct setting_category __far *parent;
    uint16_t entry_count;
    const struct setting __far* entries[];
} setting_category_t;

typedef struct setting {
    const char __far* key;
    uint16_t name;
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
            uint16_t max;
            bool (*allowed)(uint16_t);
            void (*name)(uint16_t, char *, int);
        } choice;
    };
} setting_t;

typedef struct {
    uint8_t file_flags;
    uint8_t file_sort;
    uint8_t language;
} settings_t;

extern settings_t settings;
extern const setting_category_t __far settings_root;

void settings_reset(void);
FRESULT settings_load(void);
FRESULT settings_save(void);

#endif /* _SETTINGS_H_ */
