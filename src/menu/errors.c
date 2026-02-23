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

#include <stdio.h>
#include <string.h>
#include <nilefs/ff.h>
#include "errors.h"
#include "lang.h"
#include "lang_gen.h"
#include "strings.h"

static const uint16_t __far err_swanshell_lk_table[] = {
    LK_ERROR_MCU_COMM_FAILED,
    LK_ERROR_SAVE_CORRUPT,
    LK_ERROR_MCU_BIN_CORRUPT,
    LK_ERROR_SAVE_NOR_FLASH_CORRUPT,
    LK_ERROR_FILE_TOO_LARGE,
    LK_ERROR_FILE_FORMAT_INVALID,
    LK_ERROR_DATA_TRANSFER_TIMEOUT,
    LK_ERROR_DATA_TRANSFER_CANCEL,
    LK_ERROR_FILE_NOT_EXECUTABLE,
    LK_ERROR_FLASH_COMM_FAILED,
    LK_ERROR_UNSUPPORTED_FIRMWARE_VERSION,
    LK_ERROR_INVALID_FIRMWARE_INSTALLATION,
    LK_ERROR_UNSUPPORTED_CARTRIDGE_REVISION
};

const char __far *error_to_string(int16_t value) {
    if (value >= ERR_MCU_COMM_FAILED && value < ERR_SWANSHELL_MAX) {
        return lang_keys[err_swanshell_lk_table[value - ERR_MCU_COMM_FAILED]];
    }
    switch (value) {
    case FR_INT_ERR:
        return lang_keys[LK_ERROR_FR_INT_ERR];
    case FR_DISK_ERR:
        return lang_keys[LK_ERROR_FR_DISK_ERR];
    case FR_NOT_READY:
        return lang_keys[LK_ERROR_FR_NOT_READY];
    case FR_NO_FILE:
        return lang_keys[LK_ERROR_FR_NO_FILE];
    case FR_NO_PATH:
        return lang_keys[LK_ERROR_FR_NO_PATH];
    case FR_DENIED:
        return lang_keys[LK_ERROR_FR_DENIED];
    default:
        return NULL;
    }
}

void error_to_string_buffer(int16_t value, char *buffer, size_t buflen) {
    const char __far *name = error_to_string(value);
    if (name != NULL) {
        strncpy(buffer, name, buflen);
    } else {
        snprintf(buffer, buflen, s_error_unknown, value);
    }
    buffer[buflen - 1] = '\0';
}
