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
 
#ifndef WW_HASHES_H_
#define WW_HASHES_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include "../util/hash/sha1.h"

#define WW_HASH_ENTRY_IS_LAST 0x80
#define WW_HASH_ENTRY_NAME_MAX_LENGTH 10

typedef struct __attribute__((packed)) {
    char name[WW_HASH_ENTRY_NAME_MAX_LENGTH + 1]; ///< Target filename, without file extension.
    uint16_t size; ///< Size of the hashed region, in bytes. 0 means 65536 bytes.
    uint8_t flags;
    uint8_t hash[SHA1_DIGEST_SIZE]; ///< SHA-1 hash of the region.
} ww_hash_entry_t;

extern const ww_hash_entry_t __far ww_bios_hashes[];
extern const ww_hash_entry_t __far ww_os_hashes[];

#endif /* WW_HASHES_H_ */
