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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include "fatfs/ff.h"
#include "bitmap.h"
#include "launch/launch.h"
#include "strings.h"
#include "ui.h"
#include "ui_selector.h"
#include "../util/input.h"
#include "../main.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "../../../build/menu/assets/menu/lang.h"

#define FILE_SELECTOR_ENTRY_SHIFT 8
#define FILE_SELECTOR_MAX_FILES 1536

typedef struct {
    FILINFO fno;
    uint8_t extension_loc;
} file_selector_entry_t;

static uint16_t ui_file_selector_scan_directory(void) {
    DIR dir;
    uint16_t file_count = 0;

    uint8_t result = f_opendir(&dir, ".");
	if (result != FR_OK) {
		while(1);
	}
	while (true) {
        outportw(IO_BANK_2003_RAM, file_count >> FILE_SELECTOR_ENTRY_SHIFT);
        file_selector_entry_t __far* fno = MK_FP(0x1000, file_count << FILE_SELECTOR_ENTRY_SHIFT);
		result = f_readdir(&dir, &fno->fno);
		if (result != FR_OK) {
            // TODO: error handling
			break;
		}
		if (fno->fno.fname[0] == 0) {
			break;
		}

        const char __far* ext_loc = strrchr(fno->fno.fname, '.');
        fno->extension_loc = ext_loc == NULL ? 255 : ext_loc - fno->fno.fname;

        file_count++;
        if (file_count >= FILE_SELECTOR_MAX_FILES) {
            break;
        }
	}
	f_closedir(&dir);

    return file_count;
}

static void ui_file_selector_draw(struct ui_selector_config *config, uint16_t offset, uint16_t y) {
    int x_offset = config->style == UI_SELECTOR_STYLE_16 ? 16 : 10;

    outportw(IO_BANK_2003_RAM, offset >> FILE_SELECTOR_ENTRY_SHIFT);
    file_selector_entry_t __far* fno = MK_FP(0x1000, offset << FILE_SELECTOR_ENTRY_SHIFT);

    uint16_t x = x_offset + bitmapfont_draw_string(&ui_bitmap, x_offset, y, fno->fno.fname, 224 - x_offset);
    uint8_t icon_idx = 1;
    if (fno->fno.fattrib & AM_DIR) {
        icon_idx = 0;
        bitmapfont_draw_string(&ui_bitmap, x, y, "/", 255);
    } else {
        const char __far* ext = fno->fno.fname + fno->extension_loc;
        if (ext != NULL) {
            if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc)) {
                icon_idx = 2;
            } else if (!strcasecmp(ext, s_file_ext_bmp)) {
                icon_idx = 3;
            } else if (!strcasecmp(ext, s_file_ext_wav) || !strcasecmp(ext, s_file_ext_vgm)) {
                icon_idx = 4;
            }
        }
    }

    uint16_t i = y >> 3;
    if (config->style == UI_SELECTOR_STYLE_16) {
        // TODO: swap rows and colums in 16-high icons
        if (ws_system_color_active()) {
            memcpy(MEM_TILE_4BPP(i), gfx_icons_16color + (icon_idx * 128), 32);
            memcpy(MEM_TILE_4BPP(i + 1), gfx_icons_16color + (icon_idx * 128) + 64, 32);
            memcpy(MEM_TILE_4BPP(i + 18), gfx_icons_16color + (icon_idx * 128) + 32, 32);
            memcpy(MEM_TILE_4BPP(i + 19), gfx_icons_16color + (icon_idx * 128) + 96, 32);
        } else {
            memcpy(MEM_TILE(i), gfx_icons_16mono + (icon_idx * 64), 16);
            memcpy(MEM_TILE(i + 1), gfx_icons_16mono + (icon_idx * 64) + 32, 16);
            memcpy(MEM_TILE(i + 18), gfx_icons_16mono + (icon_idx * 64) + 16, 16);
            memcpy(MEM_TILE(i + 19), gfx_icons_16mono + (icon_idx * 64) + 48, 16);
        }
    } else {
        if (ws_system_color_active()) {
            memcpy(MEM_TILE_4BPP(i + 1), gfx_icons_8color + (icon_idx * 32), 32);
        } else {
            memcpy(MEM_TILE(i + 1), gfx_icons_8mono + (icon_idx * 16), 16);
        }
    }
}

void ui_file_selector(void) {
    char path[FF_LFN_BUF + 1];

    ui_selector_config_t config = {0};
    bool reinit_ui = true;
    bool reinit_dirs = true;

    config.draw = ui_file_selector_draw;
    config.key_mask = KEY_A | KEY_B;

rescan_directory:
    if (reinit_ui) {
        ui_layout_bars();
    }
    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys_en[LK_UI_STATUS_LOADING]);
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

        if (keys_pressed & KEY_A) {
            outportw(IO_BANK_2003_RAM, config.offset >> 8);
            FILINFO __far* fno = MK_FP(0x1000, config.offset << 8);
            strncpy(path, fno->fname, sizeof(fno->fname));
            if (fno->fattrib & AM_DIR) {
                f_chdir(path);
            } else {
                const char __far* ext = strrchr(fno->altname, '.');
                if (ext != NULL) {
                    if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc)) {
                        launch_rom_metadata_t meta;
                        uint8_t result = launch_get_rom_metadata(path, &meta);
                        if (result == FR_OK) {
                            result = launch_restore_save_data(path, &meta);
                            if (result == FR_OK) {
                                result = launch_rom_via_bootstub(path, &meta);
                            }
                        }
                    } else if (!strcasecmp(ext, s_file_ext_bmp)) {
                        ui_bmpview(path);
                        reinit_ui = true;
                    } else if (!strcasecmp(ext, s_file_ext_wav)) {
                        ui_wavplay(path);
                        reinit_ui = true;
                    } else if (!strcasecmp(ext, s_file_ext_vgm)) {
                        ui_vgmplay(path);
                        reinit_ui = true;
                    }
                }
            }
            reinit_ui = true;
            reinit_dirs = true;
            goto rescan_directory;
        }
        if (keys_pressed & KEY_B) {
            f_chdir(".."); // TODO: error checking
            reinit_ui = true;
            reinit_dirs = true;
            goto rescan_directory;
        }
    }
}
