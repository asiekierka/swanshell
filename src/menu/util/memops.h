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

#ifndef UTIL_MEMOPS_H_
#define UTIL_MEMOPS_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include "config.h"

/**
 * @brief Unpack data in PSRAM, if GZIP format.
 * 
 * @param bank Pointer to source bank value; will be overwritten if unpacking is required.
 * @param dest_bank First free bank value.
 * @return int16_t 0 if successful; error code on failure.
 */
int16_t memops_unpack_psram_data_if_gzip(uint16_t *bank, uint16_t dest_bank);

#endif /* UTIL_MEMOPS_H_ */
