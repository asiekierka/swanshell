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

#include "asset_heap.h"
#include "config.h"

static uint8_t asset_heap_min_bank = (CONFIG_MAX_ROM_BANKS - 0x10 + 0x02);

uint8_t asset_heap_alloc_banks(uint8_t bank_count) {
    asset_heap_min_bank -= bank_count;
    return asset_heap_min_bank;
}

void asset_heap_free_last_banks(uint8_t bank_count) {
    asset_heap_min_bank += bank_count;
}

uint8_t asset_heap_get_free_first_banks(void) {
    return asset_heap_min_bank;
}
