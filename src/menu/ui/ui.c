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
#include "../util/input.h"
#include "../main.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "../../../build/menu/assets/menu/lang.h"

__attribute__((section(".iramx_1b80")))
uint16_t bitmap_screen2[32 * 18 - 4];

__attribute__((section(".iramx_2000")))
ws_tile_t bitmap_tiles[512];

__attribute__((section(".iramCx_4000")))
ws_tile_t bitmap_tiles_c1[512];

bitmap_t ui_bitmap;

typedef struct {
    uint16_t magic;
    uint32_t size;
    uint16_t res0, res1;
    uint32_t data_start;
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t colorplanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t data_size;
    uint32_t hres, vres;
    uint32_t color_count;
    uint32_t important_color_count;
} bmp_header_t;

void ui_draw_titlebar(const char __far* text) {
    bitmap_rect_fill(&ui_bitmap, 0, 0, 244, 8, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
    if (text != NULL) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string(&ui_bitmap, 2, 0, text, 224 - 4);
    }
}

void ui_draw_statusbar(const char __far* text) {
    bitmap_rect_fill(&ui_bitmap, 0, 144-8, 244, 8, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
    if (text != NULL) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string(&ui_bitmap, 2, 144-8, text, 224 - 4);
    }
}

void ui_draw_centered_status(const char __far* text) {
    int16_t width = bitmapfont_get_string_width(text, 224);
    bitmap_rect_clear(&ui_bitmap, 0, (144 - 12) >> 1, 224, 11);
    bitmapfont_draw_string(&ui_bitmap, (224 - width) >> 1, (144 - 12) >> 1, text, 224);
}

#define INIT_SCREEN_PATTERN(screen_loc, pal) \
    { \
        int ip = 0; \
        for (int ix = 0; ix < 28; ix++) { \
            for (int iy = 0; iy < 18; iy++) { \
                ws_screen_put_tile(screen_loc, (pal) | (ip++), ix, iy); \
            } \
        } \
    }

void ui_init(void) {
    ui_hide();

    // initialize palettes
    if (ws_system_is_color()) {
        ws_system_mode_set(WS_MODE_COLOR_4BPP);
        ui_bitmap = BITMAP(MEM_TILE_4BPP(0), 28, 18, 4);

        // palette 0 - icon palette
        MEM_COLOR_PALETTE(0)[0] = 0xFFF;
        memcpy(MEM_COLOR_PALETTE(0) + 1, gfx_icons_palcolor + 2, gfx_icons_palcolor_size - 2);

        // palette 1 - icon palette (selected)
        memcpy(MEM_COLOR_PALETTE(1) + 1, gfx_icons_palcolor + 2, gfx_icons_palcolor_size - 2);
        MEM_COLOR_PALETTE(1)[2] ^= 0xFFF;
        MEM_COLOR_PALETTE(1)[3] ^= 0xFFF;

        // palette 2 - titlebar palette
        MEM_COLOR_PALETTE(2)[0] = 0x0FFF;
        MEM_COLOR_PALETTE(2)[1] = 0x0000;
        MEM_COLOR_PALETTE(2)[2] = 0x04A7;
        MEM_COLOR_PALETTE(2)[3] = 0x0FFF;
    } else {
        ui_bitmap = BITMAP(MEM_TILE(0), 28, 18, 2);

        ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
        // palette 0 - icon palette
        outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(2, 5, 0, 7));
        // palette 1 - icon palette (selected)
        outportw(IO_SCR_PAL_1, MONO_PAL_COLORS(2, 5, 7, 0));
        // palette 2 - titlebar palette
        outportw(IO_SCR_PAL_2, MONO_PAL_COLORS(0, 7, 3, 0));
    }

    outportb(IO_SCR_BASE, SCR1_BASE(0x3800) | SCR2_BASE(bitmap_screen2));
    outportw(IO_SCR2_SCRL_X, (14*8) << 8);
}

void ui_hide(void) {
    outportw(IO_DISPLAY_CTRL, 0);
}

void ui_show(void) {
    // TODO: wallpaper support
    outportw(IO_DISPLAY_CTRL, inportw(IO_DISPLAY_CTRL) | DISPLAY_SCR2_ENABLE);
}

void ui_layout_clear(uint16_t pal) {
    bitmap_rect_clear(&ui_bitmap, 0, 0, 224, 144);
    INIT_SCREEN_PATTERN(bitmap_screen2, pal);
}

void ui_layout_bars(uint16_t icon_limit) {
    bitmap_rect_clear(&ui_bitmap, 0, 0, 224, 144);
    INIT_SCREEN_PATTERN(bitmap_screen2, (iy == 0 || iy == 17) ? SCR_ENTRY_PALETTE(2) : 0);
}

#define FILE_SELECTOR_ENTRY_SHIFT 8
#define FILE_SELECTOR_MAX_FILES 1536

#define FILE_SELECTOR_ROW_HEIGHT 16
#define FILE_SELECTOR_ROW_COUNT 8
#define FILE_SELECTOR_Y_OFFSET 8
#define FILE_SELECTOR_ROW_OFFSET 3

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

void ui_file_selector(void) {
    char path[FF_LFN_BUF + 1];
    uint16_t file_offset;
    uint16_t file_count;
    bool reinit_ui = true;
    
rescan_directory:
    if (reinit_ui) {
        ui_layout_bars(2);
        ui_show();
    }
    file_offset = 0;
    // Read files
    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys_en[LK_UI_STATUS_LOADING]);
    file_count = ui_file_selector_scan_directory();

    // Clear screen
    f_getcwd(path, sizeof(path) - 1);
    ui_draw_titlebar(path);

    bool full_redraw = true;
    uint16_t prev_file_offset = 0xFFFF;

    while (true) {
        if (prev_file_offset != file_offset) {
            if ((prev_file_offset / FILE_SELECTOR_ROW_COUNT) != (file_offset / FILE_SELECTOR_ROW_COUNT)) {
                bitmap_rect_fill(&ui_bitmap, 0, FILE_SELECTOR_Y_OFFSET, 28 * 8, FILE_SELECTOR_ROW_HEIGHT * FILE_SELECTOR_ROW_COUNT, BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
                // Draw filenames
                bitmapfont_set_active_font(font16_bitmap);
                for (int i = 0; i < FILE_SELECTOR_ROW_COUNT; i++) {
                    uint16_t offset = ((file_offset / FILE_SELECTOR_ROW_COUNT) * FILE_SELECTOR_ROW_COUNT) + i;
                    if (offset >= file_count) break;

                    outportw(IO_BANK_2003_RAM, offset >> FILE_SELECTOR_ENTRY_SHIFT);
                    file_selector_entry_t __far* fno = MK_FP(0x1000, offset << FILE_SELECTOR_ENTRY_SHIFT);

                    uint16_t x = 16 + bitmapfont_draw_string(&ui_bitmap, 16, i * FILE_SELECTOR_ROW_HEIGHT + FILE_SELECTOR_Y_OFFSET + FILE_SELECTOR_ROW_OFFSET, fno->fno.fname, 224 - 16);
                    uint8_t icon_idx = 1;
                    if (fno->fno.fattrib & AM_DIR) {
                        icon_idx = 0;
                        bitmapfont_draw_string(&ui_bitmap, x, i * FILE_SELECTOR_ROW_HEIGHT + FILE_SELECTOR_Y_OFFSET + FILE_SELECTOR_ROW_OFFSET, "/", 255);
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
                    if (FILE_SELECTOR_ROW_HEIGHT > 8) {
                        // TODO: swap rows and colums in 16-high icons
                        if (ws_system_color_active()) {
                            memcpy(MEM_TILE_4BPP(i*2 + 1), gfx_icons_16color + (icon_idx * 128), 32);
                            memcpy(MEM_TILE_4BPP(i*2 + 2), gfx_icons_16color + (icon_idx * 128) + 64, 32);
                            memcpy(MEM_TILE_4BPP(i*2 + 19), gfx_icons_16color + (icon_idx * 128) + 32, 32);
                            memcpy(MEM_TILE_4BPP(i*2 + 20), gfx_icons_16color + (icon_idx * 128) + 96, 32);
                        } else {
                            memcpy(MEM_TILE(i*2 + 1), gfx_icons_16mono + (icon_idx * 64), 16);
                            memcpy(MEM_TILE(i*2 + 2), gfx_icons_16mono + (icon_idx * 64) + 32, 16);
                            memcpy(MEM_TILE(i*2 + 19), gfx_icons_16mono + (icon_idx * 64) + 16, 16);
                            memcpy(MEM_TILE(i*2 + 20), gfx_icons_16mono + (icon_idx * 64) + 48, 16);
                        }
                    } else {
                        if (ws_system_color_active()) {
                            memcpy(MEM_TILE_4BPP(i + 1), gfx_icons_8color + (icon_idx * 32), 32);
                        } else {
                            memcpy(MEM_TILE(i + 1), gfx_icons_8mono + (icon_idx * 16), 16);
                        }
                    }
                }
                prev_file_offset = file_offset + 1;

                sprintf(path, lang_keys_en[LK_UI_FILE_SELECTOR_PAGE_FORMAT], (file_offset / FILE_SELECTOR_ROW_COUNT) + 1, ((file_count + FILE_SELECTOR_ROW_COUNT - 1) / FILE_SELECTOR_ROW_COUNT));
                ui_draw_statusbar(path);

                full_redraw = true; // FIXME
            }
            if ((prev_file_offset % FILE_SELECTOR_ROW_COUNT) != (file_offset % FILE_SELECTOR_ROW_COUNT)) {
                // Draw highlights
#if 0
                if (full_redraw) {
                    for (int iy = 0; iy < FILE_SELECTOR_ROW_COUNT; iy++) {
                        uint16_t pal = 0;
                        if (iy == (file_offset % FILE_SELECTOR_ROW_COUNT)) pal = 2;
                        bitmap_rect_fill(&ui_bitmap, 0, iy * FILE_SELECTOR_ROW_HEIGHT + FILE_SELECTOR_Y_OFFSET, 224, FILE_SELECTOR_ROW_HEIGHT, BITMAP_COLOR(pal, 2, BITMAP_COLOR_MODE_STORE));
                    }
                } else {
                    int iy1 = (prev_file_offset % FILE_SELECTOR_ROW_COUNT);
                    int iy2 = (file_offset % FILE_SELECTOR_ROW_COUNT);
                    bitmap_rect_fill(&ui_bitmap, 0, iy1 * FILE_SELECTOR_ROW_HEIGHT + FILE_SELECTOR_Y_OFFSET, 224, FILE_SELECTOR_ROW_HEIGHT, BITMAP_COLOR(0, 2, BITMAP_COLOR_MODE_STORE));
                    bitmap_rect_fill(&ui_bitmap, 0, iy2 * FILE_SELECTOR_ROW_HEIGHT + FILE_SELECTOR_Y_OFFSET, 224, FILE_SELECTOR_ROW_HEIGHT, BITMAP_COLOR(2, 2, BITMAP_COLOR_MODE_STORE));
                }
#else
                uint16_t sel = (file_offset % FILE_SELECTOR_ROW_COUNT);
                if (full_redraw) {
                    for (int ix = 0; ix < 28; ix++) {
                        for (int iy = 0; iy < 16; iy++) {
                            uint16_t pal = 0;
                            if ((iy >> (FILE_SELECTOR_ROW_HEIGHT > 8 ? 1 : 0)) == sel) pal = SCR_ATTR_PALETTE(1);
                            ws_screen_put_tile(bitmap_screen2, pal | ((iy + 1) + (ix * 18)), ix, iy + 1);
                        }
                    }
                } else {
                    uint16_t prev_sel = (prev_file_offset % FILE_SELECTOR_ROW_COUNT);
                    if (FILE_SELECTOR_ROW_HEIGHT > 8) {
                        prev_sel <<= 1;
                        sel <<= 1;
                    }
                    for (int ix = 0; ix < 28; ix++) {
                        ws_screen_put_tile(bitmap_screen2, ((prev_sel + 1) + (ix * 18)), ix, prev_sel + 1);
                        ws_screen_put_tile(bitmap_screen2, SCR_ATTR_PALETTE(1) | ((sel + 1) + (ix * 18)), ix, sel + 1);
                        
                        if (FILE_SELECTOR_ROW_HEIGHT > 8) {
                            ws_screen_put_tile(bitmap_screen2, ((prev_sel + 2) + (ix * 18)), ix, prev_sel + 2);
                            ws_screen_put_tile(bitmap_screen2, SCR_ATTR_PALETTE(1) | ((sel + 2) + (ix * 18)), ix, sel + 2);
                        }
                    }
                }
#endif
            }

            prev_file_offset = file_offset;
            full_redraw = false;
        }

        wait_for_vblank();
    	input_update();
        uint16_t keys_pressed = input_pressed;

        if (keys_pressed & KEY_X1) {
            if (file_offset == 0 || ((file_offset - 1) % FILE_SELECTOR_ROW_COUNT) == (FILE_SELECTOR_ROW_COUNT - 1))
                file_offset = file_offset + (FILE_SELECTOR_ROW_COUNT - 1);
            else
                file_offset = file_offset - 1;
        }
        if (keys_pressed & KEY_X3) {
            if ((file_offset + 1) == file_count)
                file_offset = file_offset - (file_offset % FILE_SELECTOR_ROW_COUNT);
            else if (((file_offset + 1) % FILE_SELECTOR_ROW_COUNT) == 0)
                file_offset = file_offset - (FILE_SELECTOR_ROW_COUNT - 1);
            else
                file_offset = file_offset + 1;
        }
        if (keys_pressed & KEY_X2) {
            if ((file_offset - (file_offset % FILE_SELECTOR_ROW_COUNT) + FILE_SELECTOR_ROW_COUNT) < file_count)
                file_offset += FILE_SELECTOR_ROW_COUNT;
        }
        if (keys_pressed & KEY_X4) {
            if (file_offset >= FILE_SELECTOR_ROW_COUNT)
                file_offset -= FILE_SELECTOR_ROW_COUNT;
        }
        if (file_count > 0 && file_offset >= file_count)
            file_offset = file_count - 1;
        if (keys_pressed & KEY_A) {
            outportw(IO_BANK_2003_RAM, file_offset >> 8);
            FILINFO __far* fno = MK_FP(0x1000, file_offset << 8);
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
            goto rescan_directory;
        }
        if (keys_pressed & KEY_B) {
            f_chdir(".."); // TODO: error checking
            goto rescan_directory;
        }
    }
}
