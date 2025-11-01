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

#ifndef __UI_FILE_SELECTOR_H__
#define __UI_FILE_SELECTOR_H__

#include <ws.h>
#include <nilefs.h>
#include "ui.h"
#include "ui_selector.h"
#include "../util/file.h"

#define FILE_SELECTOR_ENTRY_SHIFT 8
#define FILE_SELECTOR_MAX_FILES 1524
#define FILE_SELECTOR_RAM_BANK_OFFSET 1
#define FILE_SELECTOR_INDEX_BANK 6
#define FILE_SELECTOR_INDEXES ((uint16_t __far*) MK_FP(0x1000, 0xF410))

typedef struct {
    FILINFO fno;
    uint8_t extension_loc;
} file_selector_entry_t;

__attribute__((always_inline))
static inline bool ui_file_selector_fno_direct_same_bank(uint16_t a, uint16_t b) {
    return (a >> FILE_SELECTOR_ENTRY_SHIFT) == (b >> FILE_SELECTOR_ENTRY_SHIFT);
}

__attribute__((always_inline))
static inline file_selector_entry_t __far *ui_file_selector_open_fno_direct(uint16_t offset) {
    outportw(WS_CART_EXTBANK_RAM_PORT, FILE_SELECTOR_RAM_BANK_OFFSET + (offset >> FILE_SELECTOR_ENTRY_SHIFT));
    return MK_FP(0x1000, offset << FILE_SELECTOR_ENTRY_SHIFT);
}

__attribute__((always_inline))
static inline file_selector_entry_t __far *ui_file_selector_open_fno_direct_nobank(uint16_t offset) {
    return MK_FP(0x1000, offset << FILE_SELECTOR_ENTRY_SHIFT);
}

__attribute__((always_inline))
static inline file_selector_entry_t __far *ui_file_selector_open_fno(uint16_t offset) {
    outportw(WS_CART_EXTBANK_RAM_PORT, FILE_SELECTOR_INDEX_BANK);
    return ui_file_selector_open_fno_direct(FILE_SELECTOR_INDEXES[offset]);
}

void ui_file_selector(void);
bool ui_file_selector_default_predicate(const FILINFO __far *fno);
int16_t ui_file_selector_scan_directory(const char *path, filinfo_predicate_t predicate, uint16_t *count);

// ui_file_selector_options.c
int ui_file_selector_actions_bfb(void);
bool ui_file_selector_options(const char __far *filename, uint8_t attrib);

// ui_file_selector_qsort.c
void ui_file_selector_qsort(size_t nmemb, int (*compar)(const file_selector_entry_t __far*, const file_selector_entry_t __far*, void*), void *userdata);

#endif /* __UI_FILE_SELECTOR_H__ */
