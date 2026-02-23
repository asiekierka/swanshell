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

#ifndef ERRORS_H_
#define ERRORS_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

// Negative error codes are reserved for libc errors
#include <errno.h>

// Positive error codes < 0x80 are reserved for FatFs
#include <nilefs.h>

// Positive error codes >= 0x7F are for swanshell
#define ERR_USER_EXIT_REQUESTED 0x7F
enum {
    ERR_MCU_COMM_FAILED = 0x80,
    ERR_SAVE_CORRUPT,
    ERR_MCU_BIN_CORRUPT,
    ERR_SAVE_PSRAM_CORRUPT,
    ERR_FILE_TOO_LARGE,
    ERR_FILE_FORMAT_INVALID,
    ERR_DATA_TRANSFER_TIMEOUT,
    ERR_DATA_TRANSFER_CANCEL,
    ERR_FILE_NOT_EXECUTABLE,
    ERR_SWANSHELL_MAX
};

const char __far* error_to_string(int16_t value);
void error_to_string_buffer(int16_t value, char *buffer, size_t buflen);

#endif /* ERRORS_H_ */
