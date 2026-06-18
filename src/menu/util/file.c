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

#include <nilefs/ff.h>
#include <string.h>
#include <ws.h>
#include <ws/memory.h>
#include "file.h"
#include "errors.h"

#define INIT_SECTOR_BUFFER \
    uint8_t stack_buffer[CONFIG_MEMLAYOUT_STACK_BUFFER_SIZE]; \
    uint8_t *buffer; \
    uint16_t buffer_size; \
    if (sector_buffer_is_active()) { \
        buffer = sector_buffer; \
        buffer_size = sizeof(sector_buffer); \
    } else { \
        buffer = stack_buffer; \
        buffer_size = sizeof(stack_buffer); \
    }

bool f_anymatch(filinfo_predicate_t predicate, const char __far* local_path) {
    DIR dir;
    FILINFO fno;

    strcpy(fno.fname, local_path);
    uint8_t result = f_opendir(&dir, fno.fname);
	if (result != FR_OK)
        return false;

    while (true) {
		result = f_readdir(&dir, &fno);

        // Invalid/empty result?
		if (result != FR_OK)
            break;
		if (fno.fname[0] == 0)
			break;

        if (predicate(&fno, NULL)) {
            f_closedir(&dir);
            return true;
        }
	}

    f_closedir(&dir);
    return false;
}

bool f_exists_far(const char __far* path) {
    char local_path[FF_LFN_BUF + 1];
    local_path[FF_LFN_BUF] = 0;
    strncpy(local_path, path, FF_LFN_BUF);

    return f_stat(local_path, NULL) == FR_OK;
}

FRESULT f_open_far(FIL* fp, const char __far* path, uint8_t mode) {
    char local_path[FF_LFN_BUF + 1];
    local_path[FF_LFN_BUF] = 0;
    strncpy(local_path, path, FF_LFN_BUF);
    return f_open(fp, local_path, mode);
}

FRESULT f_unlink_far(const char __far* path) {
    char local_path[FF_LFN_BUF + 1];
    local_path[FF_LFN_BUF] = 0;
    strncpy(local_path, path, FF_LFN_BUF);

    return f_unlink(local_path);
}

int16_t f_read_sram_banked(FIL* fp, uint16_t bank, uint32_t btr, fbanked_progress_callback_t cb, void *userdata) {
    uint16_t prev_bank = inportw(WS_CART_EXTBANK_RAM_PORT);

    unsigned int lbr;
    uint32_t br = 0;
    uint32_t bytes_total = btr;

    while (btr) {
        outportw(WS_CART_EXTBANK_RAM_PORT, bank++);
        uint16_t to_read_part;
        to_read_part = btr >= 0x8000 ? 0x8000 : btr;
        btr -= to_read_part;
        int16_t result = f_read(fp, MK_FP(WS_SRAM_SEGMENT, 0x0000), to_read_part, &lbr);
        if (result != FR_OK)
            return result;
        br += lbr;
        if (cb) cb(userdata, br, bytes_total);

        to_read_part = btr >= 0x8000 ? 0x8000 : btr;
        if (to_read_part) {
            btr -= to_read_part;
            result = f_read(fp, MK_FP(WS_SRAM_SEGMENT, 0x8000), to_read_part, &lbr);
            if (result != FR_OK)
                return result;
            br += lbr;
            if (cb) cb(userdata, br, bytes_total);
        }
    }

    outportw(WS_CART_EXTBANK_RAM_PORT, prev_bank);
    return FR_OK;
}

int16_t f_write_rom_banked(FIL* fp, uint16_t bank, uint32_t btw, fbanked_progress_callback_t cb, void *userdata, bool verify) {
    uint16_t prev_bank = inportw(WS_CART_EXTBANK_ROM0_PORT);
    unsigned int lbw;
    uint32_t bytes_total = btw;

    uint16_t write_bank = bank;
    while (btw) {
        outportw(WS_CART_EXTBANK_ROM0_PORT, write_bank++);

        uint16_t to_read_part;
        to_read_part = btw >= 0x8000 ? 0x8000 : btw;
        btw -= to_read_part;
        int16_t result = f_write(fp, MK_FP(WS_ROM0_SEGMENT, 0x0000), to_read_part, &lbw);
        if (result != FR_OK)
            return result;
        if (cb != NULL) cb(userdata, bytes_total - btw, bytes_total);

        to_read_part = btw >= 0x8000 ? 0x8000 : btw;
        if (to_read_part) {
            btw -= to_read_part;
            result = f_write(fp, MK_FP(WS_ROM0_SEGMENT, 0x8000), to_read_part, &lbw);
            if (result != FR_OK)
                return result;
            if (cb != NULL) cb(userdata, bytes_total - btw, bytes_total);
        }
    }

    if (verify) {
        INIT_SECTOR_BUFFER;

        if (cb) cb(userdata, 0, bytes_total);
        f_rewind(fp);
        btw = bytes_total;
        for (uint32_t i = 0; btw > 0; i += buffer_size) {
            outportw(WS_CART_EXTBANK_ROM0_PORT, bank + (i >> 16));
            uint16_t len = buffer_size;
            if (btw < len)
                len = btw;
            uint8_t result = f_read(fp, buffer, len, &lbw);
            if (result != FR_OK)
                return result;
            if (_fmemcmp(buffer, MK_FP(WS_ROM0_SEGMENT, (uint16_t) i), len)) {
                return ERR_SAVE_VERIFY_FAILED;
            }
            if (cb) cb(userdata, bytes_total - btw, bytes_total);
            btw -= len;
        }
    }

    outportw(WS_CART_EXTBANK_ROM0_PORT, prev_bank);
    return FR_OK;
}

int16_t f_write_sram_banked(FIL* fp, uint16_t bank, uint32_t btw, fbanked_progress_callback_t cb, void *userdata, bool verify) {
    // Reading directly from SRAM would conflict with the SPI buffer.
    unsigned int lbw;
    uint32_t bytes_total = btw;
    uint16_t prev_bank = inportw(WS_CART_EXTBANK_RAM_PORT);
    INIT_SECTOR_BUFFER;

    for (uint32_t i = 0; btw > 0; i += buffer_size) {
        outportw(WS_CART_EXTBANK_RAM_PORT, bank + (i >> 16));
        uint16_t len = buffer_size;
        if (btw < len)
            len = btw;
        _fmemcpy(buffer, MK_FP(WS_SRAM_SEGMENT, (uint16_t) i), len);
        uint8_t result = f_write(fp, buffer, len, &lbw);
        if (result != FR_OK)
            return result;
        if (cb) cb(userdata, bytes_total - btw, bytes_total);
        btw -= len;
    }

    if (verify) {
        if (cb) cb(userdata, 0, bytes_total);
        f_rewind(fp);
        btw = bytes_total;
        for (uint32_t i = 0; btw > 0; i += buffer_size) {
            outportw(WS_CART_EXTBANK_RAM_PORT, bank + (i >> 16));
            uint16_t len = buffer_size;
            if (btw < len)
                len = btw;
            uint8_t result = f_read(fp, buffer, len, &lbw);
            if (result != FR_OK)
                return result;
            if (_fmemcmp(buffer, MK_FP(WS_SRAM_SEGMENT, (uint16_t) i), len)) {
                return ERR_SAVE_VERIFY_FAILED;
            }
            if (cb) cb(userdata, bytes_total - btw, bytes_total);
            btw -= len;
        }
    }

    outportw(WS_CART_EXTBANK_RAM_PORT, prev_bank);
    return FR_OK;
}
