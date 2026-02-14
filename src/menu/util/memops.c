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

#include <string.h>
#include <ws.h>
#include "../errors.h"
#include "memops.h"
#include "puff/puff.h"

__attribute__((optimize("-O0")))
int16_t memops_unpack_psram_data_if_gzip(uint16_t *bank, uint16_t dest_bank) {
    uint16_t src_bank = *bank;
    uint8_t __far* header = MK_FP(WS_ROM0_SEGMENT, 0x0000);
    
    ws_bank_with_rom0(src_bank, {
        if (((uint16_t __far*) header)[0] != 0x8B1F) {
            // not a gzip file
            return ERR_MCU_BIN_CORRUPT;
        }

        if (header[2] != 8) {
            // Not DEFLATE-compressed
            return ERR_MCU_BIN_CORRUPT;
        }
        
        if (header[3] & 0xE0) {
            // Unsupported header flags
            return ERR_MCU_BIN_CORRUPT;
        }

        uint16_t offset = 10;
        if (header[3] & 0x04) {
            // Skip FEXTRA
            offset += *((uint16_t __far*) (header + offset)) + 2;
        }
        if (header[3] & 0x08) {
            // Skip FNAME
            offset += strlen((const char*) header + offset) + 1;
        }
        if (header[3] & 0x10) {
            // Skip FCOMMENT
            offset += strlen((const char*) header + offset) + 1;
        }
        if (header[3] & 0x02) {
            // Skip FHCRC
            offset += 2;
        }

        *bank = dest_bank;
        int16_t result = FR_OK;
        ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
            ws_bank_with_ram(dest_bank, {
                if (puff(
                    MK_FP(0x1000, 0x0000),
                    (uint32_t)(0x80 - dest_bank) << 16,
                    MK_FP(WS_ROM0_SEGMENT, offset),
                    ((uint32_t)(dest_bank - src_bank) << 16) - offset
                ) < 0) {
                    result = ERR_MCU_BIN_CORRUPT;
                }
            });
        });
        return result;
    });
}