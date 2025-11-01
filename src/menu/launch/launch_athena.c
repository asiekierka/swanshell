/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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

#include "launch_athena.h"
#include <stdbool.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "errors.h"
#include "lang.h"
#include "strings.h"
#include "ui/ui_popup_dialog.h"

#define DIR_SEGMENT 0x4000
// TODO: Remove out of global allocation
static uint16_t file_segment;

typedef struct {
    uint8_t jump_command;
    uint16_t jump_offset;
    uint16_t jump_segment;
    uint8_t maintenance;
    uint16_t count;
} ww_os_footer_t;

int16_t launch_athena_begin(const char __far *bios_path, const char __far *os_path) {
    char buffer[64];
    FIL fp;
    uint32_t br;
    int16_t result;

    ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
        strcpy(buffer, bios_path);
        result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
        if (result != FR_OK) return result;

        ws_bank_with_ram(0x0F, {
            result = f_read(&fp, MK_FP(0x1000, 0x0000), 32768, &br);
            if (result != FR_OK) goto launch_athena_begin_done;
            result = f_read(&fp, MK_FP(0x1000, 0x8000), 32768, &br);
            if (result != FR_OK) goto launch_athena_begin_done;
        });

        f_close(&fp);

        strcpy(buffer, os_path);
        result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
        if (result != FR_OK) return result;

        ws_bank_with_ram(0x0E, {
            result = f_read(&fp, MK_FP(0x1000, 0x0000), 32768, &br);
            if (result != FR_OK) goto launch_athena_begin_done;
            if (!f_eof(&fp)) {
                result = f_read(&fp, MK_FP(0x1000, 0x8000), 32768, &br);
                if (result != FR_OK) goto launch_athena_begin_done;
            }

            ww_os_footer_t __far* footer = MK_FP(0x1000, 0xFFF0);
            footer->jump_command = 0xEA;
            footer->jump_offset = 0x0000;
            footer->jump_segment = 0xE000;
            footer->maintenance = 0x00;
            footer->count = (f_size(&fp) + 127) >> 7;
        });
    });

launch_athena_begin_done:
    f_close(&fp);
    return result;
}

typedef struct {
    uint16_t magic;
    uint16_t dir_segment;
    uint16_t file_count;
    uint16_t file_exec_idx;
} athena_os_footer_t;

#define ATHENA_OS_FOOTER (*((athena_os_footer_t __far*) MK_FP(0x1000, 0xFFE0)))
#define ATHENA_OS_FOOTER_BANK 0x0E
#define ATHENA_BIOS_FOOTER_BANK 0x0F

int16_t launch_athena_jump(void) {
    launch_rom_metadata_t meta = {0};
    meta.rom_type = ROM_TYPE_FREYA;
    meta.rom_banks = 16;
    meta.sram_size = 262144L;

    outportw(WS_CART_BANK_ROM0_PORT, ATHENA_BIOS_FOOTER_BANK);
    memcpy(&meta.footer, MK_FP(0x2FFF, 0x0000), 16);

    bootstub_data->prog.size = 1048576L;
    bootstub_data->prog.cluster = BOOTSTUB_CLUSTER_AT_PSRAM;

    return launch_rom_via_bootstub(&meta);
}

int16_t launch_athena_romfile_begin(void) {
    ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
        ws_bank_with_ram(ATHENA_OS_FOOTER_BANK, {
            ATHENA_OS_FOOTER.magic = 0x5AA5;
            ATHENA_OS_FOOTER.dir_segment = DIR_SEGMENT;
            ATHENA_OS_FOOTER.file_count = 0;
            ATHENA_OS_FOOTER.file_exec_idx = 0;
        });
    });

    file_segment = DIR_SEGMENT + (4 * 128);

    return FR_OK;
}

int16_t launch_athena_romfile_add(const char *path, bool is_main_executable) {
    uint16_t file_count;
    int16_t result;
    FIL fp;
    uint8_t buffer[64];
    uint32_t br;

    ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
        uint16_t prev_ram = inportw(WS_CART_EXTBANK_RAM_PORT);
        outportw(WS_CART_EXTBANK_RAM_PORT, ATHENA_OS_FOOTER_BANK);

        file_count = ATHENA_OS_FOOTER.file_count++;

        if (file_count >= 128) {
            // TODO: Custom error message?
            outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
            return FR_TOO_MANY_OPEN_FILES;
        }

        if (is_main_executable) {
            ATHENA_OS_FOOTER.file_exec_idx = file_count;
        }

        // Read file
        result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
        if (result != FR_OK) {
            outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
            return result;
        }

        result = f_read(&fp, buffer, 64, &br);
        if (result != FR_OK || *((uint32_t*) buffer) != 0x73772123) {
            if (result == FR_OK && is_main_executable) {
                result = ERR_FILE_FORMAT_INVALID;
            }
            goto launch_athena_romfile_add_done;
        }

        // Read header
        result = f_read(&fp, buffer, 64, &br);
        if (result != FR_OK) goto launch_athena_romfile_add_done;
        // Edit location
        *((uint16_t*) (buffer + 40)) = 0x0000;
        *((uint16_t*) (buffer + 42)) = file_segment;
        // Copy header to file list
        outportw(WS_CART_EXTBANK_RAM_PORT, DIR_SEGMENT >> 12);
        memcpy(MK_FP(0x1000, ((DIR_SEGMENT & 0xFFF) << 4) + (file_count * 64)), buffer, 64);

        // Read file
        uint32_t file_size = f_size(&fp) - 128;
        while (file_size) {
            uint16_t file_to_read = file_size > 512 ? 512 : file_size;

            outportw(WS_CART_EXTBANK_RAM_PORT, file_segment >> 12);
            result = f_read(&fp, MK_FP(0x1000, file_segment << 4), file_to_read, &br);
            if (result != FR_OK) goto launch_athena_romfile_add_done;

            file_segment += 512 >> 4;
            file_size -= file_to_read;
        }
        
        if (file_segment >= 0xE000) {
            result = ERR_FILE_TOO_LARGE;
        }

launch_athena_romfile_add_done:
        f_close(&fp);
        outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
        return result;
    });
}

int16_t launch_athena_boot_curdir_as_rom_wip(const char __far *name) {
    FILINFO fi;
    DIR dp;
    uint8_t buffer[FF_LFN_BUF + 1];
    buffer[0] = '.';
    buffer[1] = 0;
    int16_t result;

    ui_popup_dialog_config_t dlg = {0};
    dlg.title = lang_keys[LK_DIALOG_PREPARE_ROM];
    ui_popup_dialog_draw(&dlg);

    result = launch_athena_begin(s_path_athenabios_native, s_path_athenaos_fx);
    if (result != FR_OK) return result;

    result = launch_athena_romfile_begin();
    if (result != FR_OK) return result;

    // Read current directory for .fx/.fr/.il files
    result = f_opendir(&dp, buffer);
    if (result != FR_OK) return result;

    while (true) {
        result = f_readdir(&dp, &fi);
        if (result != FR_OK) return result;
        if (!fi.fname[0]) break;
        if (fi.fattrib & AM_DIR) continue;

        result = launch_athena_romfile_add(fi.fname, !strcmp(name, fi.fname));
        if (result != FR_OK) {
            f_closedir(&dp);
            return result;
        }
    }

    f_closedir(&dp);

    // Read /NILESWAN/fbin for .il files
    int fbin_len = strlen(s_path_fbin);
    strcpy(buffer, s_path_fbin);
    result = f_opendir(&dp, buffer);
    if (result == FR_OK) {
        while (true) {
            result = f_readdir(&dp, &fi);
            if (result != FR_OK) return result;
            if (!fi.fname[0]) break;
            if (fi.fattrib & AM_DIR) continue;

            int len = strlen(fi.fname);
            if (len >= 4 && !strcasecmp(fi.fname + len - 3, s_file_ext_il)) {
                buffer[fbin_len] = '/';
                strcpy(buffer + fbin_len + 1, fi.fname);
                result = launch_athena_romfile_add(buffer, false);
                if (result != FR_OK) {
                    f_closedir(&dp);
                    return result;
                }
            }
        }

        f_closedir(&dp);
    }
    return launch_athena_jump();
}