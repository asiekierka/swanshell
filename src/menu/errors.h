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
 
#ifndef _ERRORS_H_
#define _ERRORS_H_
 
#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

// Negative error codes are reserved for libc errors
#include <errno.h>

// Positive error codes < 0x80 are reserved for FatFs
#include <nilefs.h>

// Positive error codes >= 0x80 are for swanshell
#define ERR_MCU_COMM_FAILED 0x80
#define ERR_SAVE_CORRUPT 0x81

const char __far* error_to_string(int16_t value);
void error_to_string_buffer(int16_t value, char *buffer, size_t buflen);

#endif /* _LANG_H_ */
 