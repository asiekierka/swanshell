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

#ifndef UTIL_ASSET_HEAP_H_
#define UTIL_ASSET_HEAP_H_

#include <stdbool.h>
#include <stdint.h>
#include <ws.h>

/**
 * Allocate N banks from the PSRAM asset heap.
 * @param bank_count Number of banks to allocate.
 * @return Starting bank of allocated area.
 */
uint8_t asset_heap_alloc_banks(uint8_t bank_count);

/**
 * Free the last allocation from the PSRAM asset heap.
 * @param bank_count Number of banks to allocate.
 */
void asset_heap_free_last_banks(uint8_t bank_count);

/**
 * Get the number of free first PSRAM banks (bank 0 onwards).
 */
uint8_t asset_heap_get_free_first_banks(void);

#endif /* UTIL_ASSET_HEAP_H_ */
