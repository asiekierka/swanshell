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
#include <nilefs/ff.h>
#include <stdbool.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "errors.h"
#include "lang.h"
#include "settings.h"
#include "strings.h"
#include "ui/ui_popup_dialog.h"
#include "util/file.h"

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

typedef struct {
	/**
	 * File name, encoded in Shift-JIS
	 */
	char name[16];

	/**
	 * File description, encoded in Shift-JIS
	 */
	char info[24];

	/**
	 * Location of file in file system
	 */
	void __far* loc;

	/**
	 * Length of file, in bytes
	 */
	uint32_t len;

	/**
	 * File: 128-byte (XMODEM) chunk count, rounded up.
	 * Directory: Number of total entries in directory.
	 * (-1 = entry not allocated)
	 */
	int count;

	/**
	 * File mode.
	 */
	uint16_t mode;

	/**
	 * Modification time.
	 */
	uint32_t mtime;

	void __far *il;

	/**
	 * Offset of resource data in file, passed via the
	 * process control block to the program.
	 * (-1 = resource not present)
	 */
	int32_t resource;
} ww_fent_t;

int16_t launch_athena_begin(const char __far *bios_path, const char __far *os_path) {
    char buffer[64];
    FIL fp;
    uint32_t br;
    int16_t result;

    ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
        strcpy(buffer, bios_path);
        result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
        if (result != FR_OK) return result;

        if (f_size(&fp) != 65536L) {
            result = ERR_FILE_FORMAT_INVALID;
            goto launch_athena_begin_done;
        }

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

        if (f_size(&fp) == 0L || f_size(&fp) > 65520L) {
            result = ERR_FILE_FORMAT_INVALID;
            goto launch_athena_begin_done;
        }

        ws_bank_with_ram(0x0E, {
            result = f_read(&fp, MK_FP(0x1000, 0x0000), f_size(&fp), &br);
            if (result != FR_OK) goto launch_athena_begin_done;

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
    uint16_t ram0_file_count;
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
            ATHENA_OS_FOOTER.ram0_file_count = 0;
        });
    });

    file_segment = DIR_SEGMENT + (4 * 128);

    return FR_OK;
}

int16_t launch_athena_romfile_add(const char *path, athena_romfile_type_t type) {
    uint16_t file_count;
    int16_t result;
    FIL fp;
    uint8_t buffer[64];
    uint16_t br;
    bool read_non_ww_files = true;

    ws_bank_with_flash(WS_CART_BANK_FLASH_ENABLE, {
        uint16_t prev_ram = inportw(WS_CART_EXTBANK_RAM_PORT);
        outportw(WS_CART_EXTBANK_RAM_PORT, ATHENA_OS_FOOTER_BANK);

        if (type == ATHENA_ROMFILE_TYPE_RAM0) {
            file_count = ATHENA_OS_FOOTER.ram0_file_count++;

            if (file_count >= 64) {
                // TODO: Custom error message?
                outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
                return FR_TOO_MANY_OPEN_FILES;
            }

            file_count += ATHENA_OS_FOOTER.file_count;
        } else {
            file_count = ATHENA_OS_FOOTER.file_count++;

            if (file_count >= 128) {
                // TODO: Custom error message?
                outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
                return FR_TOO_MANY_OPEN_FILES;
            }

            if (type == ATHENA_ROMFILE_TYPE_ROM0_BOOT) {
                ATHENA_OS_FOOTER.file_exec_idx = file_count;
            }
        }

        // Read file
        result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
        if (result != FR_OK) {
            outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
            return result;
        }

        ww_fent_t *entry = (ww_fent_t*) buffer;

        result = f_read(&fp, buffer, 64, &br);
        if (result != FR_OK) goto launch_athena_romfile_add_done;
        if (*((uint32_t*) buffer) == 0x73772123) {
            // Read header
            result = f_read(&fp, buffer, 64, &br);
        } else {
            if (!read_non_ww_files || f_size(&fp) > 640L*1024L) {
                result = type == ATHENA_ROMFILE_TYPE_ROM0_BOOT ? ERR_FILE_FORMAT_INVALID : FR_OK;
                goto launch_athena_romfile_add_done;
            }

            // Seek file back to zero
            result = f_lseek(&fp, 0);

            // Create generic header
            memset(entry, 0, sizeof(ww_fent_t));
            const char *filename = (const char*) strrchr(path, '/');
            if (filename != NULL) {
                filename++;
            } else {
                filename = path;
            }
            // Name: full filename
            strncpy(entry->name, filename, 16);
            // Info: full filename
            strncpy(entry->info, filename, 24);
            entry->len = f_size(&fp);
            entry->count = (entry->len + 127) >> 7;
            entry->mode = 0;
            entry->mtime = 0; // TODO
            entry->il = NULL;
            entry->resource = -1;
        }
        if (result != FR_OK) goto launch_athena_romfile_add_done;

        // Edit location
        entry->loc = MK_FP(file_segment, 0x0000);
        entry->mode = (entry->mode & ~2) | 4; // Clear write flag, set read flag
        // Copy header to file list
        outportw(WS_CART_EXTBANK_RAM_PORT, DIR_SEGMENT >> 12);
        memcpy(MK_FP(0x1000, ((DIR_SEGMENT & 0xFFF) << 4) + (file_count * 64)), buffer, 64);

        // Read file
        uint16_t sector_count = *((uint16_t*) (buffer + 48));
        uint16_t expected_end_segment = file_segment + (sector_count << 3);

        uint32_t file_size = f_size(&fp) - f_tell(&fp);
        while (file_size) {
            uint16_t file_offset = (file_segment << 4);
            uint16_t file_max_read = (file_offset == 0) ? 32768 : -file_offset; 

            outportw(WS_CART_EXTBANK_RAM_PORT, file_segment >> 12);
            result = f_read(&fp, MK_FP(0x1000, file_offset), file_max_read, &br);
            if (result != FR_OK) goto launch_athena_romfile_add_done;

            file_size -= br;
            if ((br & 15) && file_size) {
                result = FR_INT_ERR;
                goto launch_athena_romfile_add_done;
            }

            file_segment += br >> 4;
        }

        // align to 128 bytes
        file_segment = (file_segment + 7) & ~7;
        if (file_segment < expected_end_segment)
            file_segment = expected_end_segment;

        if (file_segment > 0xE000) {
            result = ERR_FILE_TOO_LARGE;
        }
        
launch_athena_romfile_add_done:
        f_close(&fp);
        outportw(WS_CART_EXTBANK_RAM_PORT, prev_ram);
        return result;
    });
}

static const char __far path_ram0_0[] = "../ram0";
static const char __far path_ram0_1[] = "../ram";
static const char __far path_ram0_2[] = "ram0";
static const char __far path_ram0_3[] = "ram";
static const char __far * const __far path_ram0[] = {
    path_ram0_0, path_ram0_1, path_ram0_2, path_ram0_3
};
#define PATH_RAM0_COUNT (sizeof(path_ram0) / sizeof(void __far*))

int16_t launch_athena_boot_curdir_as_rom_wip(const char __far *name) {
    FILINFO fi;
    DIR dp;
    char buffer[FF_LFN_BUF + 1];
    int16_t result;
    FILINFO fno;

    buffer[0] = '.';
    buffer[1] = 0;

    ui_popup_dialog_config_t dlg = {0};
    dlg.title = lang_keys[LK_DIALOG_PREPARE_ROM];
    ui_popup_dialog_draw(&dlg);

    result = launch_athena_begin(
        settings.prog.flags & SETTING_PROG_FLAG_FX_COMPATIBLE_BIOS ? s_path_athenabios_compatible : s_path_athenabios_native, 
        s_path_athenaos_fx);
    if (result != FR_OK) return result;

    result = launch_athena_romfile_begin();
    if (result != FR_OK) return result;

    // Read current directory for .fx/.fr/.il files
    // Order:
    // i = 0 => .
    // i = 1 => /fbin
    // i = 2..N >= ../ram0, ../ram, etc.
    int fbin_len = strlen(s_path_fbin);
    for (int i = 0; i <= PATH_RAM0_COUNT+1; i++) {
        athena_romfile_type_t type = i < 2 ? ATHENA_ROMFILE_TYPE_ROM0 : ATHENA_ROMFILE_TYPE_RAM0;
        if (i >= 2) {
            // Check for ram0 path candidate
            strcpy(buffer, path_ram0[i - 2]);
            result = f_stat(buffer, &fno);
            if (result != FR_OK) continue;
            if (!(fno.fattrib & AM_DIR)) continue;

            // Found ram0 path
            result = f_chdir(buffer);
            if (result != FR_OK) return result;
        } else if (i == 1) {
            strcpy(buffer, s_path_fbin);
        }

        result = f_opendir(&dp, buffer);
        if (result != FR_OK) return result;

        while (true) {
            result = f_readdir(&dp, &fi);
            if (result != FR_OK) {
                f_closedir(&dp);
                return result;
            }
            if (!fi.fname[0]) break;
            if (fi.fattrib & AM_DIR) continue;

            athena_romfile_type_t f_type = type;
            if (i == 0) {
                if (!strcmp(name, fi.fname)) {
                    f_type = ATHENA_ROMFILE_TYPE_ROM0_BOOT;
                }
            }
            if (i == 1) {
                int len = strlen(fi.fname);
                if (len >= 4 && !strcasecmp(fi.fname + len - 3, s_file_ext_il)) {
                    buffer[fbin_len] = '/';
                    strcpy(buffer + fbin_len + 1, fi.fname);
                    result = launch_athena_romfile_add(buffer, false);
                }
            } else {
                result = launch_athena_romfile_add(fi.fname, f_type);
            }
            if (result != FR_OK) {
                f_closedir(&dp);
                return result;
            }
        }

        f_closedir(&dp);

        // Found ram0 directory; exit.
        if (i >= 2) break;
    }

    return launch_athena_jump();
}