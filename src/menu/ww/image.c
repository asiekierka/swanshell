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

#include <ctype.h>
#include <nilefs/ff.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>
#include "lang_gen.h"
#include "ww.h"
#include "hashes.h"
#include "../../shared/util/math.h"
#include "../ui/ui.h"
#include "../ui/ui_file_selector.h"
#include "../ui/ui_popup_dialog.h"
#include "../ui/ui_selector.h"
#include "../util/file.h"
#include "../errors.h"
#include "../lang.h"
#include "../settings.h"
#include "../strings.h"

static bool ww_is_bin_file(const FILINFO __far *fno) {
    if (fno->fattrib & AM_DIR)
        return false;
    
    const char __far* ext_loc = strrchr(fno->fname, '.');
    if (ext_loc == NULL || strcmp(ext_loc, s_file_ext_bin))
        return false;

    return true;
}

static bool ww_is_raw_file(const FILINFO __far *fno) {
    if (fno->fattrib & AM_DIR)
        return false;
    
    const char __far* ext_loc = strrchr(fno->fname, '.');
    if (ext_loc == NULL || strcmp(ext_loc, s_file_ext_raw))
        return false;

    return true;
}

static bool ww_is_raw_bios_file(const FILINFO __far *fno) {
    return ww_is_raw_file(fno) && !memcmp(fno->fname, s_bios, 4);
}

static bool ww_is_raw_os_file(const FILINFO __far *fno) {
    return ww_is_raw_file(fno) && memcmp(fno->fname, s_bios, 4);
}

static void ww_bios_os_selector_draw(struct ui_selector_config *config, uint16_t offset, uint16_t y) {
    char name[FF_LFN_BUF+9];

    file_selector_entry_t __far *fno = ui_file_selector_open_fno(offset);

    name[0] = 0;

    // Special handling: FreyaBIOS/FreyaOS file
    if (isdigit(fno->fno.fname[5]) && isdigit(fno->fno.fname[6]) && isdigit(fno->fno.fname[7])) {
        char __far* filename_ext = strrchr(fno->fno.fname, '.');

        if (filename_ext != NULL) *filename_ext = 0;
        if (!memcmp(s_freya, fno->fno.fname, 5)) {
            sprintf(name, s_freyaos_tpl, fno->fno.fname[5], fno->fno.fname[6], fno->fno.fname + 7);
        }
        if (!memcmp(s_bios, fno->fno.fname, 4) && fno->fno.fname[4] == 'f') {
            sprintf(name, s_freyabios_tpl, fno->fno.fname[5], fno->fno.fname[6], fno->fno.fname + 7);
        }
        if (filename_ext != NULL) *filename_ext = '.';
    }

    if (!name[0]) strcpy(name, fno->fno.fname);
    bitmapfont_draw_string(&ui_bitmap, 2, y, name, WS_DISPLAY_WIDTH_PIXELS - 2);
}

static bool ww_ui_select_file(
    bool is_os,
    char *buffer, size_t buflen
) {
    char path[32];
    strcpy(path, s_path_fbin);
    
    ui_selector_config_t config = {0};
    
    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);

    config.draw = ww_bios_os_selector_draw;
    config.key_mask = WS_KEY_A | WS_KEY_B;
    config.style = settings.file_view;

    if (ui_file_selector_scan_directory(path, is_os ? ww_is_raw_os_file : ww_is_raw_bios_file, &config.count))
        return false;

    if (config.count <= 0)
        return false;

    ui_draw_titlebar(lang_keys[LK_SELECT_FILE]);

    while (true) {
        uint16_t keys_pressed = ui_selector(&config);

        if (keys_pressed & WS_KEY_A) {
            file_selector_entry_t __far *fno = ui_file_selector_open_fno(config.offset);
            if (strlen(fno->fno.fname) >= buflen - strlen(path) - 1)
                return false;
            strcpy(buffer, s_path_fbin);
            strcat(buffer, s_path_sep);
            strcat(buffer, fno->fno.fname);
            return true;
        }
        if (keys_pressed & WS_KEY_B) {
            return false;
        }
    }
}

int16_t ww_ui_replace_component(const char __far* remote_filename, bool is_os) {
    char path[FF_LFN_BUF+1];
    char filename[FF_LFN_BUF+1];
    FIL fp;
    int16_t result;

    strcpy(filename, remote_filename);

    if (ww_ui_select_file(is_os, path, sizeof(path))) {
        char *filename_ext = (char*) strrchr(filename, '.');
        if (filename_ext == NULL)
            filename_ext = filename + strlen(filename);

        ui_popup_dialog_config_t cfg = {0};
        cfg.title = lang_keys[LK_PROGRESS_COPYING_DATA];
        cfg.progress_max = 2;
        ui_popup_dialog_draw(&cfg);

        if ((result = f_open(&fp, path, FA_READ | FA_OPEN_EXISTING)) != FR_OK) {
            return result;
        }

        uint32_t bytes_to_write;
        outportw(WS_CART_EXTBANK_RAM_PORT, 7);
        while (!f_eof(&fp)) {
            uint16_t bw;
            if ((result = f_read(&fp, MK_FP(0x1000, f_tell(&fp)), MIN(f_size(&fp), 16384), &bw)) != FR_OK) {
                f_close(&fp);
                return result;
            }

            if (f_tell(&fp) >= 65536) break;
        }
        bytes_to_write = f_tell(&fp);
        f_close(&fp);

        while (true) {
            if ((result = f_open(&fp, filename, FA_WRITE | FA_OPEN_EXISTING)) != FR_OK) {
                return result;
            }

            if (f_size(&fp) < 131072) {
                f_close(&fp);
                return ERR_FILE_FORMAT_INVALID;
            }

            if ((result = f_lseek(&fp, f_size(&fp) - (is_os ? 131072 : 65536))) != FR_OK) {
                f_close(&fp);
                return result;
            }

            uint32_t bytes_write_pos = 0;
            while (bytes_write_pos < bytes_to_write) {
                uint16_t bw;
                
                bw = MIN(bytes_to_write - bytes_write_pos, sizeof(path));
                memcpy(path, MK_FP(0x1000, f_tell(&fp)), bw);

                if ((result = f_write(&fp, path, bw, &bw)) != FR_OK) {
                    f_close(&fp);
                    return result;
                }
                bytes_write_pos += bw;
            }

            if (is_os) {
                // Create OS footer
                bytes_write_pos += 127;

                path[0] = 0xEA;
                path[1] = 0x00;
                path[2] = 0x00;
                path[3] = 0x00;
                path[4] = 0xE0;
                path[5] = 0x00;
                path[6] = bytes_write_pos >> 7;
                path[7] = bytes_write_pos >> 15;

                if ((result = f_lseek(&fp, f_size(&fp) - 65536 - 16)) != FR_OK) {
                    f_close(&fp);
                    return result;
                }

                if ((result = f_write(&fp, path, 16, &bytes_write_pos)) != FR_OK) {
                    f_close(&fp);
                    return result;
                }
            }
            
            f_close(&fp);
            
            // If the file under edit is not .flash, edit the .flash file too
            if (strcmp(filename_ext, s_file_ext_flash)) {
                strcpy(filename_ext, s_file_ext_flash);
            } else {
                break;
            }

            cfg.progress_step++;
            ui_popup_dialog_draw_update(&cfg);
        }
    }

    return 0;
}