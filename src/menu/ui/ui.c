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
#include <ws/hardware.h>
#include "fatfs/ff.h"
#include "bitmap.h"
#include "ui.h"
#include "../util/input.h"
#include "../util/util.h"
#include "../../../build/menu/assets/menu/icons.h"

__attribute__((section(".iramx_1800")))
uint16_t bitmap_screen2[32 * 18 - 4];

__attribute__((section(".iramx_2000")))
ws_tile_t bitmap_tiles[512];

__attribute__((section(".iramCx_4000")))
ws_tile_t bitmap_tiles_c1[512];

bitmap_t ui_bitmap;

void ui_init(void) {
    // initialize screen pattern
    int ip = 0;
    for (int ix = 0; ix < 28; ix++) {
        for (int iy = 0; iy < 18; iy++) {
            uint16_t pal = 0;
            if (iy == 0) pal = SCR_ATTR_PALETTE(2);
            if (iy == 17) pal = SCR_ATTR_PALETTE(3);
            ws_screen_put_tile(bitmap_screen2, pal | (ip++), ix, iy);
        }
    }

    // initialize palettes
    if (ws_system_is_color()) {
        ws_system_mode_set(WS_MODE_COLOR);
        ui_bitmap = BITMAP(MEM_TILE(0), 28, 18, 2);

        memset(MEM_TILE(0), 0, 28 * 18 * sizeof(ws_tile_t));

        MEM_COLOR_PALETTE(0)[0] = 0x0FFF;
        MEM_COLOR_PALETTE(0)[1] = 0x0000;
        MEM_COLOR_PALETTE(0)[2] = 0x0555;
        MEM_COLOR_PALETTE(0)[3] = 0x0AAA;

        MEM_COLOR_PALETTE(1)[0] = 0x0AAA;
        MEM_COLOR_PALETTE(1)[1] = 0x0000;
        MEM_COLOR_PALETTE(1)[2] = 0x0222;
        MEM_COLOR_PALETTE(1)[3] = 0x0777;

        MEM_COLOR_PALETTE(2)[0] = 0x04A7;
        MEM_COLOR_PALETTE(2)[1] = 0x0FFF;
        MEM_COLOR_PALETTE(3)[0] = 0x04A7;
        MEM_COLOR_PALETTE(3)[1] = 0x0FFF;
    } else {
        ui_bitmap = BITMAP(MEM_TILE(0), 28, 18, 2);

        memset(MEM_TILE(0), 0, 28 * 18 * sizeof(ws_tile_t));

        ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
        outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 5, 2));
        outportw(IO_SCR_PAL_1, MONO_PAL_COLORS(2, 7, 6, 4));
        outportw(IO_SCR_PAL_2, MONO_PAL_COLORS(4, 0, 0, 0));
        outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(4, 0, 0, 0));
    }

    outportb(IO_SCR_BASE, SCR2_BASE(bitmap_screen2));
    outportw(IO_SCR2_SCRL_X, 0);
    outportw(IO_DISPLAY_CTRL, DISPLAY_SCR2_ENABLE);
}

#define MAX_FILE_COUNT 1024

static const char __far ext_bmp[] = ".bmp";
static const char __far ext_vgm[] = ".vgm";
static const char __far ext_wav[] = ".wav";
static const char __far ext_ws[] = ".ws";
static const char __far ext_wsc[] = ".wsc";

void ui_file_selector(void) {
    char path[FF_LFN_BUF + 1];
    DIR dir;
    uint16_t file_count;
    uint16_t file_offset;
    
rescan_directory:
    file_count = 0;
    file_offset = 0;
    // Read files
    bitmap_rect_clear(&ui_bitmap, 0, 17 * 8, 28 * 8, 8);
    bitmapfont_draw_string(&ui_bitmap, 2, 17 * 8, "Reading...", 224 - 2);
	uint8_t result = f_opendir(&dir, ".");
	if (result != FR_OK) {
		while(1);
	}
	while (true) {
        outportw(IO_BANK_2003_RAM, file_count >> 7);
        FILINFO __far* fno = MK_FP(0x1000, file_count << 9);
		result = f_readdir(&dir, fno);
		if (result != FR_OK) {
            // TODO: error handling
			break;
		}
		if (fno->fname[0] == 0) {
			break;
		}
        file_count++;
        if (file_count >= MAX_FILE_COUNT) {
            break;
        }
	}
	f_closedir(&dir);

    // Clear screen
    bitmap_rect_clear(&ui_bitmap, 0, 0, 28 * 8, 8);
    strcpy(path, "Listing ");
    f_getcwd(path + 8, sizeof(path) - 9);
    bitmapfont_draw_string(&ui_bitmap, 2, 0, path, 224 - 2);

    bool full_redraw = true;
    uint16_t prev_file_offset = 0xFFFF;

    while (true) {
        if (prev_file_offset != file_offset) {
            if ((prev_file_offset & 0xFFF0) != (file_offset & 0xFFF0)) {
                bitmap_rect_clear(&ui_bitmap, 0, 8, 28 * 8, 16 * 8);
                // Draw filenames
                for (int i = 0; i < 16; i++) {
                    uint16_t offset = (file_offset & 0xFFF0) | i;
                    if (offset >= file_count) break;

                    outportw(IO_BANK_2003_RAM, offset >> 7);
                    FILINFO __far* fno = MK_FP(0x1000, offset << 9);

                    uint16_t x = 9 + bitmapfont_draw_string(&ui_bitmap, 9, (i + 1) * 8, fno->fname, 224 - 10);
                    uint8_t icon_idx = 1;
                    if (fno->fattrib & AM_DIR) {
                        icon_idx = 0;
                        bitmapfont_draw_string(&ui_bitmap, x, (i + 1) * 8, "/", 255);
                    } else {
                        const char __far* ext = strrchr(fno->altname, '.');
                        if (ext != NULL) {
                            if (!strcasecmp(ext, ext_ws) || !strcasecmp(ext, ext_wsc)) {
                                icon_idx = 2;
                            } else if (!strcasecmp(ext, ext_bmp)) {
                                icon_idx = 3;
                            } else if (!strcasecmp(ext, ext_wav) || !strcasecmp(ext, ext_vgm)) {
                                icon_idx = 4;
                            }
                        }
                    }
                    memcpy(MEM_TILE(i + 1), gfx_icons_tiles + (icon_idx * 16), 16);
                }
            }
            if ((prev_file_offset & 0xF) != (file_offset & 0xF)) {
                // Draw highlights
                if (full_redraw) {
                    for (int ix = 0; ix < 28; ix++) {
                        for (int iy = 0; iy < 16; iy++) {
                            uint16_t pal = 0;
                            if (iy == (file_offset & 0xF)) pal = SCR_ATTR_PALETTE(1);
                            ws_screen_put_tile(bitmap_screen2, pal | ((iy + 1) + (ix * 18)), ix, iy + 1);
                        }
                    }
                } else {
                    int iy1 = (prev_file_offset & 0xF);
                    int iy2 = (file_offset & 0xF);
                    for (int ix = 0; ix < 28; ix++) {
                        ws_screen_put_tile(bitmap_screen2, ((iy1 + 1) + (ix * 18)), ix, iy1 + 1);
                        ws_screen_put_tile(bitmap_screen2, SCR_ATTR_PALETTE(1) | ((iy2 + 1) + (ix * 18)), ix, iy2 + 1);
                    }
                }
            }
            
            bitmap_rect_clear(&ui_bitmap, 0, 17 * 8, 28 * 8, 8);
            sprintf(path, "Page %d/%d", (file_offset >> 4) + 1, ((file_count + 15) >> 4));
            bitmapfont_draw_string(&ui_bitmap, 2, 17 * 8, path, 224 - 2);
            prev_file_offset = file_offset;
            full_redraw = false;
        }

        wait_for_vblank();
    	input_update();
        uint16_t keys_pressed = input_pressed;

        if (keys_pressed & KEY_X1) {
            file_offset = ((file_offset - 1) & 0xF) | (file_offset & 0xFFF0);
            if (file_count > 0 && file_offset >= file_count)
                file_offset = file_count - 1;
        }
        if (keys_pressed & KEY_X3) {
            file_offset = ((file_offset + 1) & 0xF) | (file_offset & 0xFFF0);
            if (file_count > 0 && file_offset >= file_count)
                file_offset = (file_offset & 0xFFF0);
        }
        if (keys_pressed & KEY_X2) {
            if (((file_offset + 16) & ~0xF) < file_count)
                file_offset += 16;
            if (file_count > 0 && file_offset >= file_count)
                file_offset = file_count - 1;
        }
        if (keys_pressed & KEY_X4) {
            if (file_offset >= 16)
                file_offset -= 16;
        }
        if (keys_pressed & KEY_A) {
            outportw(IO_BANK_2003_RAM, file_offset >> 7);
            FILINFO __far* fno = MK_FP(0x1000, file_offset << 9);
            strncpy(path, fno->fname, sizeof(fno->fname));
            if (fno->fattrib & AM_DIR) {
                f_chdir(path);
            } else {
                const char __far* ext = strrchr(fno->altname, '.');
                if (ext != NULL) {
                    if (!strcasecmp(ext, ext_ws) || !strcasecmp(ext, ext_wsc)) {
                        ui_boot(path);
                    } else if (!strcasecmp(ext, ext_bmp)) {
                        ui_bmpview(path);
                    } else if (!strcasecmp(ext, ext_wav)) {
                        ui_wavplay(path);
                    } else if (!strcasecmp(ext, ext_vgm)) {
                        ui_vgmplay(path);
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
