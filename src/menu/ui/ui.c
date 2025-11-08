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
#include <wsx/planar_convert.h>
#include "bitmap.h"
#include "strings.h"
#include "ui.h"
#include "util/bmp.h"
#include "util/file.h"
#include "util/input.h"
#include "main.h"
#include "settings.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "lang.h"
#include "config.h"

__attribute__((section(".iramx_1b80")))
uint16_t bitmap_screen2[32 * 18 - 4];

__attribute__((section(".iramx_2000")))
ws_display_tile_t bitmap_tiles[512];
#define bitmap_screen1 ((uint16_t ws_iram*) 0x3800)

__attribute__((section(".iramCx_4000")))
ws_display_tile_t bitmap_tiles_c1[512];

bitmap_t ui_bitmap;

void ui_draw_titlebar(const char __far* text) {
    bitmap_rect_fill(&ui_bitmap, 0, 0, WS_DISPLAY_WIDTH_PIXELS, 8, BITMAP_COLOR_2BPP(2));
    if (text != NULL) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string(&ui_bitmap, 2, 0, text, WS_DISPLAY_WIDTH_PIXELS - 4);
    }
}

void ui_draw_statusbar(const char __far* text) {
    bitmap_rect_fill(&ui_bitmap, 0, WS_DISPLAY_HEIGHT_PIXELS-8, WS_DISPLAY_WIDTH_PIXELS, 8, BITMAP_COLOR_2BPP(2));
    bitmapfont_set_active_font(font8_bitmap);
    if (text != NULL) {
        bitmapfont_draw_string(&ui_bitmap, 2, WS_DISPLAY_HEIGHT_PIXELS-8, text, WS_DISPLAY_WIDTH_PIXELS - 4);
    }
}

void ui_draw_centered_status(const char __far* text) {
    int16_t width = bitmapfont_get_string_width(text, WS_DISPLAY_WIDTH_PIXELS);
    bitmap_rect_clear(&ui_bitmap, 0, (WS_DISPLAY_HEIGHT_PIXELS - 12) >> 1, WS_DISPLAY_WIDTH_PIXELS, 11);
    bitmapfont_draw_string(&ui_bitmap, (WS_DISPLAY_WIDTH_PIXELS - width) >> 1, (WS_DISPLAY_HEIGHT_PIXELS - 12) >> 1, text, WS_DISPLAY_WIDTH_PIXELS);
}

#define INIT_SCREEN_PATTERN(screen_loc, pal, ofs) \
    { \
        int ip = (ofs); \
        for (int ix = 0; ix < 28; ix++) { \
            for (int iy = 0; iy < 18; iy++) { \
                ws_screen_put_tile(screen_loc, (pal) | (ip++), ix, iy); \
            } \
        } \
    }

void ui_init(void) {
    ui_hide();

    // initialize palettes
#ifdef CONFIG_DEBUG_FORCE_MONO
    if (0) {
#else
    if (ws_system_is_color_model()) {
#endif
        ws_system_set_mode(WS_MODE_COLOR_4BPP);
        ui_bitmap = BITMAP(WS_TILE_4BPP_MEM(0), 28, 18, 4);

        // palette 0 - icon palette
        WS_DISPLAY_COLOR_MEM(0)[0] = 0xFFF;
        ws_gdma_copyp(WS_DISPLAY_COLOR_MEM(0) + 2, gfx_icons_palcolor + 4, gfx_icons_palcolor_size - 4);
        WS_DISPLAY_COLOR_MEM(0)[1] = 0x000;

        // palette 1 - icon palette (selected)
        ws_gdma_copyp(WS_DISPLAY_COLOR_MEM(1) + 2, gfx_icons_palcolor + 4, gfx_icons_palcolor_size - 4);
        WS_DISPLAY_COLOR_MEM(1)[1] = 0xFFF;
        WS_DISPLAY_COLOR_MEM(1)[2] ^= 0xFFF;
        WS_DISPLAY_COLOR_MEM(1)[3] ^= 0xFFF;

        // palette 2 - titlebar palette
        WS_DISPLAY_COLOR_MEM(2)[0] = 0xFFF;
        WS_DISPLAY_COLOR_MEM(2)[1] = 0x000;
        WS_DISPLAY_COLOR_MEM(2)[2] = SETTING_THEME_ACCENT_COLOR_DEFAULT;
        WS_DISPLAY_COLOR_MEM(2)[3] = 0xFFF;
    } else {
        ui_bitmap = BITMAP(WS_TILE_MEM(0), 28, 18, 2);

        ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT_DEFAULT);
        // palette 0 - icon palette
        outportw(WS_SCR_PAL_0_PORT, 0x7052);
        // palette 1 - icon palette (selected)
        outportw(WS_SCR_PAL_1_PORT, 0x0725);
        // palette 2 - titlebar palette
        outportw(WS_SCR_PAL_2_PORT, 0x0370);
    }

    outportb(WS_SCR_BASE_PORT, WS_SCR_BASE_ADDR1(0x3800) | WS_SCR_BASE_ADDR2(bitmap_screen2));
    outportw(WS_SCR2_SCRL_X_PORT, (14*8) << 8);
}

void ui_hide(void) {
    outportw(WS_DISPLAY_CTRL_PORT, 0);
}

#ifdef CONFIG_ENABLE_WALLPAPER
// 0 - not checked, 2 - checked, not found; 1 - checked, found
static uint8_t wallpaper_status = 0;

bool ui_has_wallpaper(void) {
    return wallpaper_status == 1;
}
#endif

__attribute__((always_inline))
static inline void ui_show_inner(void) {
#ifdef CONFIG_ENABLE_WALLPAPER
    outportw(WS_DISPLAY_CTRL_PORT, inportw(WS_DISPLAY_CTRL_PORT) | WS_DISPLAY_CTRL_SCR2_ENABLE | (wallpaper_status & 1));
#else
    outportw(WS_DISPLAY_CTRL_PORT, inportw(WS_DISPLAY_CTRL_PORT) | WS_DISPLAY_CTRL_SCR2_ENABLE);
#endif
}

#ifdef CONFIG_ENABLE_WALLPAPER
static inline void load_wallpaper(void) {
    FIL fp;
    uint16_t br;

    wallpaper_status = 2;
    if (ws_system_get_mode() != WS_MODE_COLOR_4BPP) return;

    int16_t result = f_open_far(&fp, s_path_wallpaper_bmp, FA_READ | FA_OPEN_EXISTING);
    if (result != FR_OK || f_size(&fp) > 65535) return;

    INIT_SCREEN_PATTERN(bitmap_screen1, WS_SCREEN_ATTR_PALETTE(3), WS_SCREEN_ATTR_BANK(1));
    // memset(WS_TILE_4BPP_MEM(512), 0, 28 * 18 * 32);

    ui_show_inner();

    result = f_read(&fp, MK_FP(0x1000, 0x0000), f_size(&fp), &br);
    f_close(&fp);
    if (result != FR_OK) return;

    bmp_header_t __far* bmp = MK_FP(0x1000, 0x0000);
    if (bmp->magic != 0x4d42 || bmp->header_size < 40 ||
        bmp->width != WS_DISPLAY_WIDTH_PIXELS || bmp->height != WS_DISPLAY_HEIGHT_PIXELS ||
        bmp->compression != 0 || bmp->bpp != 4) return;

    uint8_t __far *palette = MK_FP(0x1000, 14 + bmp->header_size);
    for (int i = 0; i < 16; i++) {
        uint8_t b = *(palette++);
        uint8_t g = *(palette++);
        uint8_t r = *(palette++);
        palette++;
        WS_DISPLAY_COLOR_MEM(3)[i] = WS_RGB(r >> 4, g >> 4, b >> 4);
    }
    WS_DISPLAY_COLOR_MEM(0)[0] = WS_DISPLAY_COLOR_MEM(3)[0];

    uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
    uint16_t pitch = (((bmp->width * bmp->bpp) + 31) / 32) << 2;
    for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
        uint32_t __far *line_src = (uint32_t __far*) data;
        uint32_t *line_dst = (uint32_t*) (0x8000 + (((uint16_t)bmp->height - 1 - y) * 4));
        for (uint8_t x = 0; x < bmp->width; x += 8, line_src++, line_dst += 18 * 8) {
            *line_dst = wsx_planar_convert_4bpp_packed_row(*line_src);
        }
    }

    wallpaper_status = 1;
}
#endif

void ui_show(void) {
#ifdef CONFIG_ENABLE_WALLPAPER
    if (!wallpaper_status) {
        load_wallpaper();
    }
#endif
    ui_show_inner();
}

void ui_layout_clear(uint16_t pal) {
    if (pal == 0 && !ui_has_wallpaper()) {
        bitmap_rect_fill(&ui_bitmap, 0, 0, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS, BITMAP_COLOR_4BPP(2));
    } else {
        bitmap_rect_clear(&ui_bitmap, 0, 0, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS);
    }
    INIT_SCREEN_PATTERN(bitmap_screen2, pal, 0);
}

void ui_layout_bars(void) {
    if (!ui_has_wallpaper()) {
        bitmap_rect_fill(&ui_bitmap, 0, 8, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS - 16, BITMAP_COLOR_4BPP(2));
    } else {
        bitmap_rect_clear(&ui_bitmap, 0, 8, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS - 16);
    }
    INIT_SCREEN_PATTERN(bitmap_screen2, (iy == 0 || iy == 17) ? WS_SCREEN_ATTR_PALETTE(2) : 0, 0);
}
