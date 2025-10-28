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

#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "bootstub.h"
#include "../patches.h"

extern uint8_t freya_soft_reset;
extern uint8_t freya_soft_reset_end;

#define FREYA_SOFT_RESET_START 0xFFC8

void patch_apply_freya_soft_reset(uint16_t last_bank) {
    outportw(WS_CART_EXTBANK_RAM_PORT, last_bank);
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);

    // Check if the preceding area is unused.
    // The patch area could already be overwritten, so this is a kind of heuristic.
    for (int i = 0; i < 32; i++) {
        if (*((uint16_t __far*) MK_FP(0x1000, 0xFF80 + (i << 1))) != 0xFFFF)
            return;
    }

    memcpy(MK_FP(0x1000, FREYA_SOFT_RESET_START), &freya_soft_reset, ((uint16_t) &freya_soft_reset_end) - ((uint16_t) &freya_soft_reset));
    memcpy(&bootstub_data->start_pointer, MK_FP(0x1000, 0xFFF1), 4);
    *((uint16_t __far*) MK_FP(0x1000, 0xFFF1)) = FREYA_SOFT_RESET_START;
    *((uint16_t __far*) MK_FP(0x1000, 0xFFF3)) = 0xF000;

    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
}
