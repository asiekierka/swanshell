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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>
#include "bitmap.h"
#include "lang_gen.h"
#include "launch/launch.h"
#include "settings.h"
#include "strings.h"
#include "ui.h"
#include "ui/ui_file_selector.h"
#include "ui/ui_popup_list.h"
#include "ui_error.h"
#include "ui_selector.h"
#include "ui_settings.h"
#include "../util/input.h"
#include "../main.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "lang.h"

#define COMPARE(a, b) (((a) == (b)) ? 0 : (((a) < (b)) ? -1 : 1))

static int compare_filenames(const file_selector_entry_t __far* a, const file_selector_entry_t __far* b, void *userdata) {
    uint8_t mode = (uint8_t) userdata;
    bool a_dir = a->fno.fattrib & AM_DIR;
    bool b_dir = b->fno.fattrib & AM_DIR;
    if (a_dir && !b_dir) {
        return -1;
    } else if (!a_dir && b_dir) {
        return 1;
    } else if (mode == SETTING_FILE_SORT_NAME_ASC) {
        return strcasecmp(a->fno.fname, b->fno.fname);
    } else if (mode == SETTING_FILE_SORT_NAME_DESC) {
        return strcasecmp(b->fno.fname, a->fno.fname);
    } else if (mode == SETTING_FILE_SORT_DATE_ASC) {
        return COMPARE(a->fno.fdate, b->fno.fdate) || COMPARE(a->fno.ftime, b->fno.ftime);
    } else if (mode == SETTING_FILE_SORT_DATE_DESC) {
        return COMPARE(b->fno.fdate, a->fno.fdate) || COMPARE(b->fno.ftime, a->fno.ftime);
    } else if (mode == SETTING_FILE_SORT_SIZE_ASC) {
        return COMPARE(a->fno.fsize, b->fno.fsize);
    } else if (mode == SETTING_FILE_SORT_SIZE_DESC) {
        return COMPARE(b->fno.fsize, a->fno.fsize);
    } else {
        return 0;
    }
}

static uint16_t ui_file_selector_scan_directory(void) {
    DIR dir;
    uint16_t file_count = 0;

    uint8_t result = f_opendir(&dir, ".");
	if (result != FR_OK) {
		while(1);
	}
	while (true) {
        file_selector_entry_t __far* fno = ui_file_selector_open_fno_direct(file_count);
		result = f_readdir(&dir, &fno->fno);

        // Invalid/empty result?
		if (result != FR_OK) {
            // TODO: error handling
			break;
		}
		if (fno->fno.fname[0] == 0) {
			break;
		}

        // Hidden file?
        if ((fno->fno.fattrib & AM_HID) && !(settings.file_flags & SETTING_FILE_SHOW_HIDDEN)) {
            continue;
        }

        // Cache extension location
        const char __far* ext_loc = strrchr(fno->fno.fname, '.');
        fno->extension_loc = ext_loc == NULL ? 255 : ext_loc - fno->fno.fname;

        file_count++;
        if (file_count >= FILE_SELECTOR_MAX_FILES) {
            break;
        }
	}
	f_closedir(&dir);

    outportw(WS_CART_EXTBANK_RAM_PORT, FILE_SELECTOR_INDEX_BANK);
    for (int i = 0; i < file_count; i++)
        FILE_SELECTOR_INDEXES[i] = i;
    ui_file_selector_qsort(file_count, compare_filenames, (void*) settings.file_sort);

    return file_count;
}

static void ui_file_selector_draw(struct ui_selector_config *config, uint16_t offset, uint16_t y) {
    int x_offset = config->style == UI_SELECTOR_STYLE_16 ? 16 : 10;

    file_selector_entry_t __far *fno = ui_file_selector_open_fno(offset);
    uint16_t x = x_offset + bitmapfont_draw_string(&ui_bitmap, x_offset, y, fno->fno.fname, WS_DISPLAY_WIDTH_PIXELS - x_offset);
    uint8_t icon_idx = 1;
    if (fno->fno.fattrib & AM_DIR) {
        icon_idx = 0;
        bitmapfont_draw_char(&ui_bitmap, x + BITMAPFONT_CHAR_GAP, y, '/');
    } else {
        const char __far* ext = fno->fno.fname + fno->extension_loc;
        if (ext != NULL) {
            if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc)) {
                icon_idx = 2;
            } else if (!strcasecmp(ext, s_file_ext_bmp)) {
                icon_idx = 3;
            } else if (!strcasecmp(ext, s_file_ext_wav) || !strcasecmp(ext, s_file_ext_vgm)) {
                icon_idx = 4;
            } else if (!strcasecmp(ext, s_file_ext_bfb)) {
                icon_idx = 5;
            }
        }
    }

    uint16_t i = y >> 3;
    if (config->style == UI_SELECTOR_STYLE_16) {
        // TODO: swap rows and colums in 16-high icons
        if (ws_system_is_color_active()) {
            memcpy(WS_TILE_4BPP_MEM(i), gfx_icons_16color + (icon_idx * 128), 32);
            memcpy(WS_TILE_4BPP_MEM(i + 1), gfx_icons_16color + (icon_idx * 128) + 64, 32);
            memcpy(WS_TILE_4BPP_MEM(i + 18), gfx_icons_16color + (icon_idx * 128) + 32, 32);
            memcpy(WS_TILE_4BPP_MEM(i + 19), gfx_icons_16color + (icon_idx * 128) + 96, 32);
        } else {
            memcpy(WS_TILE_MEM(i), gfx_icons_16mono + (icon_idx * 64), 16);
            memcpy(WS_TILE_MEM(i + 1), gfx_icons_16mono + (icon_idx * 64) + 32, 16);
            memcpy(WS_TILE_MEM(i + 18), gfx_icons_16mono + (icon_idx * 64) + 16, 16);
            memcpy(WS_TILE_MEM(i + 19), gfx_icons_16mono + (icon_idx * 64) + 48, 16);
        }
    } else {
        if (ws_system_is_color_active()) {
            memcpy(WS_TILE_4BPP_MEM(i), gfx_icons_8color + (icon_idx * 32), 32);
        } else {
            memcpy(WS_TILE_MEM(i), gfx_icons_8mono + (icon_idx * 16), 16);
        }
    }
}

static bool ui_file_selector_options(void) {
    ui_popup_list_config_t lst = {0};
    lst.option[0] = lang_keys[LK_SUBMENU_OPTION_SETTINGS];
    switch (ui_popup_list(&lst)) {
    default:
        return false;
    case 0:
        ui_settings(&settings_root);
        return true;
    }
}

static int ui_file_selector_bfb_options(void) {
    ui_popup_list_config_t lst = {0};
    lst.option[0] = lang_keys[LK_SUBMENU_OPTION_TEST];
    return ui_popup_list(&lst);
}

void ui_file_selector(void) {
    char path[FF_LFN_BUF + 1];

    ui_selector_config_t config = {0};
    bool reinit_ui = true;
    bool reinit_dirs = true;

rescan_directory:
    config.draw = ui_file_selector_draw;
    config.key_mask = WS_KEY_A | WS_KEY_B | WS_KEY_START;
    config.style = settings.file_view;

    if (reinit_ui) {
        ui_layout_bars();
    }
    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);
    ui_show();
    if (reinit_dirs) {
        config.offset = 0;
        config.count = ui_file_selector_scan_directory();
    }
    if (reinit_ui || reinit_dirs) {
        f_getcwd(path, sizeof(path) - 1);
        ui_draw_titlebar(path);
    }

    reinit_ui = false;
    reinit_dirs = false;

    while (true) {
        uint16_t keys_pressed = ui_selector(&config);

        if (keys_pressed & WS_KEY_A) {
            file_selector_entry_t __far *fno = ui_file_selector_open_fno(config.offset);
            strncpy(path, fno->fno.fname, sizeof(fno->fno.fname));
            if (fno->fno.fattrib & AM_DIR) {
                f_chdir(path);
            } else {
                ui_selector_clear_selection(&config);
                if (fno->extension_loc != 255) {
                    const char __far* ext = fno->fno.fname + fno->extension_loc;
                    if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc)) {
                        launch_rom_metadata_t meta;
                        int16_t result = launch_get_rom_metadata(path, &meta);
                        if (result == FR_OK) {
                            result = launch_restore_save_data(path, &meta);
                            if (result == FR_OK) {
                                result = launch_rom_via_bootstub(path, &meta);
                            }
                        }

                        // Error
                        ui_error_handle(result, NULL, 0);
                        reinit_ui = true;
                        goto rescan_directory;
                    } else if (!strcasecmp(ext, s_file_ext_bmp)) {
                        ui_error_handle(ui_bmpview(path), NULL, 0);
                    } else if (!strcasecmp(ext, s_file_ext_wav)) {
                        ui_error_handle(ui_wavplay(path), NULL, 0);
                        reinit_ui = true;
                        goto rescan_directory;
                    } else if (!strcasecmp(ext, s_file_ext_vgm)) {
                        ui_error_handle(ui_vgmplay(path), NULL, 0);
                        reinit_ui = true;
                        goto rescan_directory;
                    } else if (!strcasecmp(ext, s_file_ext_bfb)) {
                        ui_selector_clear_selection(&config);
                        int option = ui_file_selector_bfb_options();
                        if (option == 0) {
                            ui_error_handle(launch_bfb(path), NULL, 0);
                        }
                        reinit_ui = true;
                        goto rescan_directory;
                    }
                }
            }
            reinit_ui = true;
            reinit_dirs = true;
            goto rescan_directory;
        }
        if (keys_pressed & WS_KEY_B) {
            f_chdir(".."); // TODO: error checking
            reinit_ui = true;
            reinit_dirs = true;
            goto rescan_directory;
        }

        // TODO: temporary
        if (keys_pressed & WS_KEY_START) {
            ui_selector_clear_selection(&config);
            if (ui_file_selector_options()) {
                reinit_dirs = true;
            }
            reinit_ui = true;
            goto rescan_directory;
        }
    }
}
