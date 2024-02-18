/**
 * Copyright (c) 2024 Adrian Siekierka
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
#include "file.h"

uint8_t f_read_sram_banked(FIL* fp, uint16_t bank, uint32_t btr, uint32_t *br) {
    uint16_t prev_bank = inportw(IO_BANK_2003_RAM);
    
    uint16_t lbr;
    if (br != NULL)
        *br = 0;

    while (btr) {
        outportw(IO_BANK_2003_RAM, bank++);
        uint16_t to_read_part;
        to_read_part = btr >= 0x8000 ? 0x8000 : btr;
        btr -= to_read_part;
        uint8_t result = f_read(fp, MK_FP(0x1000, 0x0000), to_read_part, &lbr);
        if (result != FR_OK)
            return result;
        if (br != NULL)
            *br += lbr;

        to_read_part = btr >= 0x8000 ? 0x8000 : btr;
        if (to_read_part) {
            btr -= to_read_part;
            result = f_read(fp, MK_FP(0x1000, 0x8000), to_read_part, &lbr);
            if (result != FR_OK)
                return result;
            if (br != NULL)
                *br += lbr;
        }
    }

    outportw(IO_BANK_2003_RAM, prev_bank);
    return FR_OK;
}

uint8_t f_write_rom_banked(FIL* fp, uint16_t bank, uint32_t btw, uint32_t *bw) {
    uint16_t prev_bank = inportw(IO_BANK_2003_ROM0);
    uint16_t lbw;
    if (bw != NULL)
        *bw = 0;

    while (btw) {
        outportw(IO_BANK_2003_ROM0, bank++);
        uint16_t to_read_part;
        to_read_part = btw >= 0x8000 ? 0x8000 : btw;
        btw -= to_read_part;
        uint8_t result = f_write(fp, MK_FP(0x2000, 0x0000), to_read_part, &lbw);
        if (result != FR_OK)
            return result;
        if (bw != NULL)
            *bw += lbw;

        to_read_part = btw >= 0x8000 ? 0x8000 : btw;
        if (to_read_part) {
            btw -= to_read_part;
            result = f_write(fp, MK_FP(0x2000, 0x8000), to_read_part, &lbw);
            if (result != FR_OK)
                return result;
            if (bw != NULL)
                *bw += lbw;
        }
    }

    outportw(IO_BANK_2003_ROM0, prev_bank);
    return FR_OK;
}

uint8_t f_write_sram_banked(FIL* fp, uint16_t bank, uint32_t btw, uint32_t *bw) {
    // Reading directly from SRAM would conflict with the SPI buffer.
    uint8_t stack_buffer[16];
    uint8_t *buffer;
    uint16_t buffer_size;
    uint16_t lbw;

    if (ws_system_color_active()) {
        buffer = sector_buffer;
        buffer_size = sizeof(sector_buffer);
    } else {
        buffer = stack_buffer;
        buffer_size = sizeof(stack_buffer);
    }

    uint16_t prev_bank = inportw(IO_BANK_2003_RAM);
    if (bw != NULL)
        *bw = 0;

    for (uint32_t i = 0; i < btw; i += buffer_size, btw -= buffer_size) {
        outportw(IO_BANK_2003_RAM, bank + (i >> 16));
        uint16_t len = buffer_size;
        if (btw < len)
            len = btw;
        _fmemcpy(buffer, MK_FP(0x1000, (uint16_t) i), len);
        uint8_t result = f_write(fp, buffer, len, &lbw);
        if (result != FR_OK)
            return result;
        if (bw != NULL)
            *bw += lbw;
    }

    outportw(IO_BANK_2003_RAM, prev_bank);
    return FR_OK;
}
