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
#include "ui.h"
#include "../util/input.h"
#include "../main.h"
#include "../../../build/menu/assets/menu/icons.h"

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

static const char __far wallpaper_path[] = "/wallpaper.bmp";
static void ui_load_wallpaper(void) {
    FIL fp;
    char path[20];
    strcpy(path, wallpaper_path);

    uint8_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) {
        return;
    }

    if (f_size(&fp) > 65535) {
        f_close(&fp);
        return;
    }

    result = f_read(&fp, MK_FP(0x1000, 0x0000), f_size(&fp), NULL);
    f_close(&fp);
    if (result != FR_OK) {
        return;
    }

    bmp_header_t __far* bmp = MK_FP(0x1000, 0x0000);
    if (bmp->magic != 0x4d42 || bmp->header_size < 40 ||
        bmp->width != 224 || bmp->height != 144 ||
        bmp->compression != 0 || bmp->color_count > 16 || bmp->bpp != 4) {
        return;
    }

    // configure palette
    uint8_t __far *palette = MK_FP(0x1000, 14 + bmp->header_size);
    for (int i = 0; i < bmp->color_count; i++) {
        uint8_t b = *(palette++) >> 4;
        uint8_t g = *(palette++) >> 4;
        uint8_t r = *(palette++) >> 4;
        b = (b>>1) + 8; if (b > 15) b = 15;
        g = (g>>1) + 8; if (g > 15) g = 15;
        r = (r>>1) + 8; if (r > 15) r = 15;
        palette++;
        MEM_COLOR_PALETTE(15)[i] = RGB(r, g, b);
    }
    MEM_COLOR_PALETTE(0)[0] = MEM_COLOR_PALETTE(15)[0];

    outportw(IO_DISPLAY_CTRL, inportw(IO_DISPLAY_CTRL) | DISPLAY_SCR1_ENABLE);

    // copy data
    uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
    uint16_t pitch = (((bmp->width * bmp->bpp) + 31) / 32) << 2;
    for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
        uint32_t __far *line_src = (uint32_t __far*) data;
        uint32_t *line_dst = (uint32_t*) (0x8000 + (((uint16_t)bmp->height - 1 - y) * 4));
        for (uint8_t x = 0; x < bmp->width; x += 8, line_src++, line_dst += 18 * 8) {
            *line_dst = bitmap_c2p_4bpp_pixel(*line_src);
        }
    }
}

void ui_draw_titlebar(void) {
    bitmap_rect_fill(&ui_bitmap, 3, 2+1, 224 - 6, 16, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
    bitmap_rect_fill(&ui_bitmap, 2, 3+1, 1, 14, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
    bitmap_rect_fill(&ui_bitmap, 223 - 2, 3+1, 1, 14, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
}

void ui_redraw_titlebar(const char *text) {
    bitmap_rect_fill(&ui_bitmap, 3, 2+1, 224 - 6, 16, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
    if (text != NULL)
        bitmapfont_draw_string(&ui_bitmap, 6, 4+1, text, 224 - 12);
}

void ui_init(void) {
    outportw(IO_DISPLAY_CTRL, 0);

    // initialize screen pattern
    int ip = 0;
    for (int ix = 0; ix < 28; ix++) {
        for (int iy = 0; iy < 18; iy++) {
            uint16_t pal = 0;
            if (iy <= 2) pal = SCR_ATTR_PALETTE(2);
           ws_screen_put_tile(bitmap_screen2, pal | (ip++), ix, iy);   
        }
    }

    // initialize palettes
    if (ws_system_is_color()) {
#if 1
        ws_system_mode_set(WS_MODE_COLOR_4BPP);
        ui_bitmap = BITMAP(MEM_TILE_4BPP(0), 28, 18, 4);
#else
        ws_system_mode_set(WS_MODE_COLOR);
        ui_bitmap = BITMAP(MEM_TILE(0), 28, 18, 2);
#endif

        MEM_COLOR_PALETTE(0)[0] = 0x0FFF;
        MEM_COLOR_PALETTE(0)[1] = 0x0000;
        MEM_COLOR_PALETTE(0)[2] = 0x0000;
        MEM_COLOR_PALETTE(0)[3] = 0x0FFF;

        MEM_COLOR_PALETTE(1)[0] = 0x0AAA;
        MEM_COLOR_PALETTE(1)[1] = 0x0000;
        MEM_COLOR_PALETTE(1)[2] = 0x0222;
        MEM_COLOR_PALETTE(1)[3] = 0x0777;

        MEM_COLOR_PALETTE(2)[0] = 0x0FFF;
        MEM_COLOR_PALETTE(2)[1] = 0x0000;
        MEM_COLOR_PALETTE(2)[2] = 0x04A7;
        MEM_COLOR_PALETTE(2)[3] = 0x0FFF;

        // ui_load_wallpaper();
    } else {
        ui_bitmap = BITMAP(MEM_TILE(0), 28, 18, 2);

        ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
        outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 5, 2));
        outportw(IO_SCR_PAL_1, MONO_PAL_COLORS(2, 7, 6, 4));
        outportw(IO_SCR_PAL_2, MONO_PAL_COLORS(0, 7, 3, 0));
    }
    
    bitmap_rect_clear(&ui_bitmap, 0, 0, 224, 144);

    ui_draw_titlebar();

    outportb(IO_SCR_BASE, SCR1_BASE(0x3800) | SCR2_BASE(bitmap_screen2));
    outportw(IO_SCR2_SCRL_X, (14*8) << 8);
    outportw(IO_DISPLAY_CTRL, inportw(IO_DISPLAY_CTRL) | DISPLAY_SCR2_ENABLE);
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
    ui_redraw_titlebar(NULL);
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
    f_getcwd(path, sizeof(path) - 1);
    ui_redraw_titlebar(path);

    bool full_redraw = true;
    uint16_t prev_file_offset = 0xFFFF;

    while (true) {
        if (prev_file_offset != file_offset) {
            if ((prev_file_offset / 10) != (file_offset / 10)) {
                bitmap_rect_clear(&ui_bitmap, 0, 24, 28 * 8, 120);
                // Draw filenames
                for (int i = 0; i < 10; i++) {
                    uint16_t offset = ((file_offset / 10) * 10) + i;
                    if (offset >= file_count) break;

                    outportw(IO_BANK_2003_RAM, offset >> 7);
                    FILINFO __far* fno = MK_FP(0x1000, offset << 9);

                    uint16_t x = 9 + bitmapfont_draw_string(&ui_bitmap, 9, (i + 2) * 12, fno->fname, 224 - 10);
                    uint8_t icon_idx = 1;
                    if (fno->fattrib & AM_DIR) {
                        icon_idx = 0;
                        bitmapfont_draw_string(&ui_bitmap, x, (i + 2) * 12, "/", 255);
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
                    // memcpy(MEM_TILE(i + 1), gfx_icons_tiles + (icon_idx * 16), 16);
                }
                prev_file_offset = file_offset + 1;
            }
            if ((prev_file_offset % 10) != (file_offset % 10)) {
                // Draw highlights
#if 1
                if (full_redraw) {
                    for (int iy = 0; iy < 10; iy++) {
                        uint16_t pal = 0;
                        if (iy == (file_offset % 10)) pal = 2;
                        bitmap_rect_fill(&ui_bitmap, 0, (iy + 2) * 12, 224, 12, BITMAP_COLOR(pal, 2, BITMAP_COLOR_MODE_STORE));
                    }
                } else {
                    int iy1 = (prev_file_offset % 10);
                    int iy2 = (file_offset % 10);
                    bitmap_rect_fill(&ui_bitmap, 0, (iy1 + 2) * 12, 224, 12, BITMAP_COLOR(0, 2, BITMAP_COLOR_MODE_STORE));
                    bitmap_rect_fill(&ui_bitmap, 0, (iy2 + 2) * 12, 224, 12, BITMAP_COLOR(2, 2, BITMAP_COLOR_MODE_STORE));
                }
#else
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
#endif
            }
            
            // bitmap_rect_fill(&ui_bitmap, 0, 17 * 8, 28 * 8, 8, BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
            // sprintf(path, "Page %d/%d", (file_offset >> 4) + 1, ((file_count + 15) >> 4));
            // bitmapfont_draw_string(&ui_bitmap, 2, 17 * 8, path, 224 - 2);
            prev_file_offset = file_offset;
            full_redraw = false;
        }

        wait_for_vblank();
    	input_update();
        uint16_t keys_pressed = input_pressed;

        if (keys_pressed & KEY_X1) {
            if (file_offset == 0 || ((file_offset - 1) % 10) == 9)
                file_offset = file_offset + 9;
            else
                file_offset = file_offset - 1;
        }
        if (keys_pressed & KEY_X3) {
            if ((file_offset + 1) == file_count)
                file_offset = file_offset - (file_offset % 10);
            else if (((file_offset + 1) % 10) == 0)
                file_offset = file_offset - 9;
            else
                file_offset = file_offset + 1;
        }
        if (keys_pressed & KEY_X2) {
            if ((file_offset - (file_offset % 10) + 10) < file_count)
                file_offset += 10;
        }
        if (keys_pressed & KEY_X4) {
            if (file_offset >= 10)
                file_offset -= 10;
        }
        if (file_count > 0 && file_offset >= file_count)
            file_offset = file_count - 1;
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
                        launch_rom_metadata_t meta;
                        uint8_t result = launch_get_rom_metadata(path, &meta);
                        if (result == FR_OK) {
                            result = launch_restore_save_data(path, &meta);
                            if (result == FR_OK) {
                                result = launch_rom_via_bootstub(path, NULL);
                            }
                        }
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
