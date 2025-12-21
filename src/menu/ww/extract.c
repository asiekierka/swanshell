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

#include <nilefs/ff.h>
#include <stddef.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>
#include "lang_gen.h"
#include "launch/launch.h"
#include "ww.h"
#include "hashes.h"
#include "../../shared/util/math.h"
#include "../ui/ui.h"
#include "../ui/ui_popup_dialog.h"
#include "../util/file.h"
#include "../util/hash/sha1.h"
#include "../errors.h"
#include "../lang.h"
#include "../strings.h"

#define PROGRESS_DIALOG_SHIFT 4

static int16_t find_target_filename(
    const ww_hash_entry_t __far *hash,
    FIL *fp,
    const ww_hash_entry_t __far **result,
    uint16_t lang_key
) {
    SHA1_CTX context, saved_context;
    uint8_t sha_result[SHA1_DIGEST_SIZE];
    uint32_t bytes_read = 0;
    ui_popup_dialog_config_t dlg = {0};

    uint8_t *buffer;
    uint16_t buffer_size;

    if (sector_buffer_is_active()) {
        buffer = sector_buffer;
        buffer_size = sizeof(sector_buffer);
    } else {
        buffer = saved_context.buffer;
        buffer_size = 64;
    }
    if (buffer_size > 2048)
        buffer_size = 2048;

    dlg.title = lang_keys[lang_key];
    dlg.progress_max = 65536 >> PROGRESS_DIALOG_SHIFT;

    ui_popup_dialog_draw(&dlg);

    *result = NULL;
    SHA1_Init(&context);

    while (true) {
        uint32_t bytes_to_read = hash->size ? hash->size : 65536;
        while (bytes_read < bytes_to_read) {
            unsigned int bytes_read_now = 0;
            uint16_t bytes_to_read_now = MIN(bytes_to_read - bytes_read, buffer_size);

            int16_t result = f_read(fp, buffer, bytes_to_read_now, &bytes_read_now);
            if (result != FR_OK)
                return result;

            SHA1_Update(&context, buffer, bytes_read_now);
            bytes_read += bytes_read_now;

            dlg.progress_step = bytes_read >> PROGRESS_DIALOG_SHIFT;
            ui_popup_dialog_draw_update(&dlg);
        }

        memcpy(&saved_context, &context, sizeof(SHA1_CTX));
        SHA1_Final(&context, sha_result);

        if (!memcmp(sha_result, hash->hash, SHA1_DIGEST_SIZE)) {
            *result = hash;
            ui_popup_dialog_clear(&dlg);
            return 0;
        }

        if (hash->flags & WW_HASH_ENTRY_IS_LAST) {
            ui_popup_dialog_clear(&dlg);
            return 0;
        }

        memcpy(&context, &saved_context, sizeof(SHA1_CTX));
        hash++;
    }
}

static int16_t create_ww_paths(void) {
    char path[32];

    strcpy(path, s_path_fbin);
    int16_t result = f_mkdir(path);
    if (result != FR_OK && result != FR_EXIST)
        return result;

    return FR_OK;
}

static int16_t ww_copy_to_file(FIL *fp, const char __far* filename, uint16_t _size, uint16_t lang_key) {
    FIL outfp;
    uint32_t size = _size ? _size : 65536;
    char local_buffer[64];
    int16_t result;
    ui_popup_dialog_config_t dlg = {0};

    uint8_t *buffer;
    uint16_t buffer_size;

    if (sector_buffer_is_active()) {
        buffer = sector_buffer;
        buffer_size = sizeof(sector_buffer);
    } else {
        buffer = (uint8_t*) local_buffer;
        buffer_size = sizeof(local_buffer);
    }

    dlg.title = lang_keys[lang_key];
    dlg.progress_max = size >> PROGRESS_DIALOG_SHIFT;

    ui_popup_dialog_draw(&dlg);

    strcpy(local_buffer, s_path_fbin);
    strcat(local_buffer, s_path_sep);
    strcat(local_buffer, filename);
    strcat(local_buffer, s_file_ext_raw);

    if ((result = f_open(&outfp, local_buffer, FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK) {
        return result;
    }

    while (size) {
        uint16_t to_copy = MIN(size, buffer_size);
        unsigned int copied;
        if ((result = f_read(fp, buffer, to_copy, &copied)) != FR_OK) {
            f_close(&outfp);
            return result;
        }
        if ((result = f_write(&outfp, buffer, copied, &copied)) != FR_OK) {
            f_close(&outfp);
            return result;
        }

        dlg.progress_step = dlg.progress_max - (size >> PROGRESS_DIALOG_SHIFT);
        ui_popup_dialog_draw_update(&dlg);

        size -= copied;
    }

    ui_popup_dialog_clear(&dlg);

    f_close(&outfp);
    return FR_OK;
}

int16_t ww_ui_extract_from_rom(const char __far* filename) {
    FIL fp;
    int16_t result;
    const ww_hash_entry_t __far *found_entry;
    rom_footer_t bios_footer;
    rom_footer_t os_footer;

    if ((result = create_ww_paths()) != FR_OK)
        return result;

    if ((result = f_open_far(&fp, filename, FA_OPEN_EXISTING | FA_READ)) != FR_OK)
        return result;

    if (f_size(&fp) < 131072) {
        f_close(&fp);
        return ERR_FILE_FORMAT_INVALID;
    }

    // read BIOS footer
    if ((result = f_lseek(&fp, f_size(&fp) - 16)) != FR_OK) {
        f_close(&fp);
        return result;
    }
    if ((result = f_read(&fp, &bios_footer, 16, &result) != FR_OK)) {
        f_close(&fp);
        return result;
    }

    // check basic footer values
    if (bios_footer.jump_command != 0xEA
        || bios_footer.jump_segment < 0xF000
        || bios_footer.maintenance & 0x0F) {

        f_close(&fp);
        return ERR_FILE_FORMAT_INVALID;
    }

    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SUBMENU_OPTION_WITCH_EXTRACT_BIOS_OS]);
    ui_draw_statusbar(NULL);

    // seek to OS location
    if ((result = f_lseek(&fp, f_size(&fp) - 131072L)) != FR_OK) {
        f_close(&fp);
        return result;
    }
    if ((result = find_target_filename(ww_os_hashes, &fp, &found_entry, LK_WITCH_EXTRACT_PROGRESS_OS_SCANNING)) != FR_OK) {
        f_close(&fp);
        return result;
    }

    if (found_entry != NULL) {
        if ((result = f_lseek(&fp, f_size(&fp) - 131072L)) != FR_OK) {
            f_close(&fp);
            return result;
        }

        if ((result = ww_copy_to_file(&fp, found_entry->name, found_entry->size, LK_WITCH_EXTRACT_PROGRESS_OS_EXTRACTING)) != FR_OK) {
            f_close(&fp);
            return result;
        }
    } else {
        /* if ((result = f_lseek(&fp, f_size(&fp) - 65536L - 16)) != FR_OK) {
            f_close(&fp);
            return result;
        }
        if ((result = f_read(&fp, &os_footer, 16, &result) != FR_OK)) {
            f_close(&fp);
            return result;
        } */

        // TODO: handle unknown OS versions
    }

    // seek to BIOS location
    if ((result = f_lseek(&fp, f_size(&fp) - 65536L)) != FR_OK) {
        f_close(&fp);
        return result;
    }
    if ((result = find_target_filename(ww_bios_hashes, &fp, &found_entry, LK_WITCH_EXTRACT_PROGRESS_BIOS_SCANNING)) != FR_OK) {
        f_close(&fp);
        return result;
    }

    if (found_entry != NULL) {
        if ((result = f_lseek(&fp, f_size(&fp) - 65536L)) != FR_OK) {
            f_close(&fp);
            return result;
        }

        if ((result = ww_copy_to_file(&fp, found_entry->name, found_entry->size, LK_WITCH_EXTRACT_PROGRESS_BIOS_EXTRACTING)) != FR_OK) {
            f_close(&fp);
            return result;
        }
    } else {
        // TODO: handle unknown BIOS versions
    }

    f_close(&fp);
    return 0;
}
