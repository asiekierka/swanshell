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

const char __far *error_to_string(int16_t value) {
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
    case ERR_MCU_COMM_FAILED:
        return lang_keys[LK_ERROR_MCU_COMM_FAILED];
    case ERR_SAVE_CORRUPT:
        return lang_keys[LK_ERROR_SAVE_CORRUPT];
    case ERR_MCU_BIN_CORRUPT:
        return lang_keys[LK_ERROR_MCU_BIN_CORRUPT];
    case ERR_SAVE_PSRAM_CORRUPT:
        return lang_keys[LK_ERROR_SAVE_NOR_FLASH_CORRUPT];
    case ERR_FILE_TOO_LARGE:
        return lang_keys[LK_ERROR_FILE_TOO_LARGE];
    case ERR_FILE_FORMAT_INVALID:
        return lang_keys[LK_ERROR_FILE_FORMAT_INVALID];
    case ERR_DATA_TRANSFER_TIMEOUT:
        return lang_keys[LK_ERROR_DATA_TRANSFER_TIMEOUT];
    case ERR_DATA_TRANSFER_CANCEL:
        return lang_keys[LK_ERROR_DATA_TRANSFER_CANCEL];
    case ERR_FILE_NOT_EXECUTABLE:
        return lang_keys[LK_ERROR_FILE_NOT_EXECUTABLE];
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
