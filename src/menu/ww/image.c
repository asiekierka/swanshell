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
#include "util/input.h"
#include "ww.h"
#include "../../shared/util/math.h"
#include "../ui/ui.h"
#include "../ui/ui_fileops.h"
#include "../ui/ui_file_selector.h"
#include "../ui/ui_osk.h"
#include "../ui/ui_popup_dialog.h"
#include "../ui/ui_selector.h"
#include "../util/file.h"
#include "../errors.h"
#include "../lang.h"
#include "../settings.h"
#include "../strings.h"

#define PROGRESS_DIALOG_READ_SHIFT 5
#define PROGRESS_DIALOG_WRITE_SHIFT 2

static bool ww_is_bin_file(const FILINFO __far *fno) {
    if (fno->fattrib & AM_DIR)
        return false;
    
    const char __far* ext_loc = strrchr(fno->fname, '.');
    if (ext_loc == NULL || strcasecmp(ext_loc, s_file_ext_bin))
        return false;

    return true;
}

static bool ww_is_raw_file(const FILINFO __far *fno) {
    if (fno->fattrib & AM_DIR)
        return false;
    
    const char __far* ext_loc = strrchr(fno->fname, '.');
    if (ext_loc == NULL || strcasecmp(ext_loc, s_file_ext_raw))
        return false;

    return true;
}

static bool ww_is_raw_bios_file(const FILINFO __far *fno) {
    return ww_is_raw_file(fno) && !memcmp(fno->fname, s_bios, 4);
}

static bool ww_is_raw_os_file(const FILINFO __far *fno) {
    return ww_is_raw_file(fno) && memcmp(fno->fname, s_bios, 4);
}

#define WW_UI_SELECT_HAS_BIOSATHC 0x0001
#define WW_UI_SELECT_HAS_BIOSATHN 0x0002
#define WW_UI_SELECT_BIOSATHC -1
#define WW_UI_SELECT_BIOSATHN -2

static int16_t get_ui_select_file_offset(uint16_t flags, uint16_t offset) {
    uint16_t extra_entries = __builtin_popcount(flags);
    if (offset >= extra_entries) {
        return offset - extra_entries;
    }

    if (flags & WW_UI_SELECT_HAS_BIOSATHC) {
        if (!offset) return WW_UI_SELECT_BIOSATHC;
        offset--;
    }

    if (flags & WW_UI_SELECT_HAS_BIOSATHN) {
        if (!offset) return WW_UI_SELECT_BIOSATHN;
        offset--;
    }

    // Should never reach this.
    while(1);
}

// FIXME: compiler bug?
__attribute__((optimize("-O1")))
static void ww_bios_os_selector_draw(struct ui_selector_config *config, uint16_t ui_offset, uint16_t y) {
    char name[FF_LFN_BUF+9];
    name[0] = 0;

    int16_t offset = get_ui_select_file_offset((uint16_t) config->userdata, ui_offset);
    if (offset >= 0) {
        file_selector_entry_t __far *fno = ui_file_selector_open_fno((uint16_t) offset);

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
    } else {
        // TODO: Implement version information
        strcpy(name, s_athenabios_tpl);
        strcat(name, lang_keys[offset == WW_UI_SELECT_BIOSATHC ? LK_ATHENABIOS_SUFFIX_COMPATIBLE : LK_ATHENABIOS_SUFFIX_NATIVE]);
    }
    bitmapfont_draw_string(&ui_bitmap, 2, y, name, WS_DISPLAY_WIDTH_PIXELS - 2);
}

// FIXME: compiler bug?
__attribute__((optimize("-O1")))
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
        config.count = 0;

    uint16_t flags = 0;
    if (!is_os) {
        if (f_exists_far(s_path_athenabios_compatible))
            flags |= WW_UI_SELECT_HAS_BIOSATHC;
        if (f_exists_far(s_path_athenabios_native))
            flags |= WW_UI_SELECT_HAS_BIOSATHN;
    }
    config.userdata = (void*) flags;
    config.count += __builtin_popcount(flags);

    if (config.count <= 0)
        return false;

    ui_draw_titlebar(lang_keys[LK_SELECT_FILE]);

    while (true) {
        uint16_t keys_pressed = ui_selector(&config);

        if (keys_pressed & WS_KEY_A) {
            int16_t offset = get_ui_select_file_offset(flags, config.offset);

            if (offset >= 0) {
                file_selector_entry_t __far *fno = ui_file_selector_open_fno(offset);
                if (strlen(fno->fno.fname) >= buflen - strlen(path) - 1)
                    return false;
                strcpy(buffer, s_path_fbin);
                strcat(buffer, s_path_sep);
                strcat(buffer, fno->fno.fname);
            } else if (offset == WW_UI_SELECT_BIOSATHC) {
                strcpy(buffer, s_path_athenabios_compatible);
            } else if (offset == WW_UI_SELECT_BIOSATHN) {
                strcpy(buffer, s_path_athenabios_native);
            } else {
                return false;
            }

            return true;
        }
        if (keys_pressed & WS_KEY_B) {
            return false;
        }
    }
}

static int16_t ww_ui_replace_component_path(char *input_path, char *output_path, bool is_os, uint32_t size) {
    uint8_t local_buffer[64];
    int16_t result;
    FIL fp;

    bool dynamic_size = size == 0;
    char *output_ext = (char*) strrchr(output_path, '.');
    if (output_ext == NULL)
        output_ext = output_path + strlen(output_path);

    ui_popup_dialog_config_t cfg = {0};
    cfg.title = lang_keys[LK_PROGRESS_COPYING_DATA];
    cfg.progress_max = (65536L >> PROGRESS_DIALOG_READ_SHIFT) + (65536L >> PROGRESS_DIALOG_WRITE_SHIFT);
    ui_popup_dialog_draw(&cfg);

    if ((result = f_open(&fp, input_path, FA_READ | FA_OPEN_EXISTING)) != FR_OK) {
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

        cfg.progress_step = (f_tell(&fp) >> PROGRESS_DIALOG_READ_SHIFT);
        ui_popup_dialog_draw_update(&cfg);

        if (f_tell(&fp) >= 65536) break;
    }
    bytes_to_write = f_tell(&fp);
    f_close(&fp);

    uint8_t *buffer;
    uint16_t buffer_size;

    if (sector_buffer_is_active()) {
        buffer = sector_buffer;
        buffer_size = sizeof(sector_buffer);
    } else {
        buffer = local_buffer;
        buffer_size = sizeof(local_buffer);
    }

    bool ignore_file_not_existing = false;

    while (true) {
        if ((result = f_open(&fp, output_path, FA_WRITE | FA_OPEN_EXISTING)) != FR_OK) {
            if (ignore_file_not_existing && (result == FR_NO_FILE || result == FR_NO_PATH))
                break;
            return result;
        }

        if (dynamic_size)
            size = f_size(&fp);
        if (size < 131072) {
            f_close(&fp);
            return ERR_FILE_FORMAT_INVALID;
        }

        if ((result = f_lseek(&fp, size - (is_os ? 131072 : 65536))) != FR_OK) {
            f_close(&fp);
            return result;
        }

        uint32_t bytes_write_pos = 0;
        while (bytes_write_pos < bytes_to_write) {
            uint16_t bw;
            
            bw = MIN(bytes_to_write - bytes_write_pos, buffer_size);
            memcpy(buffer, MK_FP(0x1000, f_tell(&fp)), bw);

            if ((result = f_write(&fp, buffer, bw, &bw)) != FR_OK) {
                f_close(&fp);
                return result;
            }

            cfg.progress_step = (65536L >> PROGRESS_DIALOG_READ_SHIFT) + (bytes_write_pos >> PROGRESS_DIALOG_WRITE_SHIFT);
            ui_popup_dialog_draw_update(&cfg);

            bytes_write_pos += bw;
        }

        if (is_os) {
            // Create OS footer
            bytes_write_pos += 127;

            input_path[0] = 0xEA;
            input_path[1] = 0x00;
            input_path[2] = 0x00;
            input_path[3] = 0x00;
            input_path[4] = 0xE0;
            input_path[5] = 0x00;
            input_path[6] = bytes_write_pos >> 7;
            input_path[7] = bytes_write_pos >> 15;

            if ((result = f_lseek(&fp, size - 65536 - 16)) != FR_OK) {
                f_close(&fp);
                return result;
            }

            if ((result = f_write(&fp, input_path, 16, &bytes_write_pos)) != FR_OK) {
                f_close(&fp);
                return result;
            }
        }
        
        f_close(&fp);
        
        // If the file under edit is not .flash, edit the .flash file too
        if (!dynamic_size)
            break;
        if (strcasecmp(output_ext, s_file_ext_flash)) {
            strcpy(output_ext, s_file_ext_flash);
        } else {
            break;
        }

        ignore_file_not_existing = true;
    }

    return 0;
}

int16_t ww_ui_replace_component(const char __far* remote_filename, bool is_os) {
    char path[FF_LFN_BUF+1];
    char filename[FF_LFN_BUF+1];

    strcpy(filename, remote_filename);
    if (ww_ui_select_file(is_os, path, sizeof(path))) {
        return ww_ui_replace_component_path(path, filename, is_os, 0);
    }

    return 0;
}

int16_t ww_ui_create_image(void) {
    char image_path[FF_LFN_BUF+1];
    char bios_path[FF_LFN_BUF+1];
    char os_path[FF_LFN_BUF+1];
    FIL fp;
    int16_t result;

    // Query for BIOS/OS path
    if (!ww_ui_select_file(false, bios_path, sizeof(bios_path))) return 0;
    if (!ww_ui_select_file(true, os_path, sizeof(os_path))) return 0;

    // Query for image path name
    ui_osk_state_t osk = {0};
    image_path[0] = 0;
    osk.buffer = image_path;
    osk.buflen = sizeof(image_path);

    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_INPUT_TARGET_FILENAME]);
    ui_draw_statusbar(NULL);
    ui_osk(&osk);

    if (!fileops_is_rom(image_path))
        strcat(image_path, s_file_ext_wsc);

    if ((result = ui_fileops_check_file_overwrite(image_path)) != FR_OK) {
        if (result == FR_EXIST) return FR_OK;
        return result;
    }

    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SUBMENU_OPTION_WITCH_CREATE_IMAGE]);
    ui_draw_statusbar(NULL);
    // Allocate file
    if ((result = f_open(&fp, image_path, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
        return result;

    // Try to ensure a contiguous area for the file.
    result = f_expand(&fp, 524288L, 0);
    /// ... failing that, non-contiguous will do.
    if (result != FR_OK)
        result = f_lseek(&fp, 524288L);
    f_close(&fp);
    if (result != FR_OK)
        return result;

    // Inject BIOS/OS data
    if ((result = ww_ui_replace_component_path(bios_path, image_path, false, 524288L)) != FR_OK)
        return result;
    if ((result = ww_ui_replace_component_path(os_path, image_path, true, 524288L)) != FR_OK)
        return result;

    return FR_OK;
}