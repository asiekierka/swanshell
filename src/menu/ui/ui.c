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
#include "bitmap.h"
#include "launch/launch.h"
#include "strings.h"
#include "ui.h"
#include "ui_selector.h"
#include "../util/input.h"
#include "../main.h"
#include "settings.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "lang.h"
#include "config.h"

__attribute__((section(".iramx_1b80")))
uint16_t bitmap_screen2[32 * 18 - 4];

__attribute__((section(".iramx_2000")))
ws_display_tile_t bitmap_tiles[512];

__attribute__((section(".iramCx_4000")))
ws_display_tile_t bitmap_tiles_c1[512];

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
    bitmap_rect_fill(&ui_bitmap, 0, 0, WS_DISPLAY_WIDTH_PIXELS, 8, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
    if (text != NULL) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string(&ui_bitmap, 2, 0, text, WS_DISPLAY_WIDTH_PIXELS - 4);
    }
}

void ui_draw_statusbar(const char __far* text) {
    bitmap_rect_fill(&ui_bitmap, 0, WS_DISPLAY_HEIGHT_PIXELS-8, WS_DISPLAY_WIDTH_PIXELS, 8, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
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
        memcpy(WS_DISPLAY_COLOR_MEM(0) + 1, gfx_icons_palcolor + 2, gfx_icons_palcolor_size - 2);

        // palette 1 - icon palette (selected)
        memcpy(WS_DISPLAY_COLOR_MEM(1) + 1, gfx_icons_palcolor + 2, gfx_icons_palcolor_size - 2);
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

void ui_show(void) {
    // TODO: wallpaper support
    outportw(WS_DISPLAY_CTRL_PORT, inportw(WS_DISPLAY_CTRL_PORT) | WS_DISPLAY_CTRL_SCR2_ENABLE);
}

void ui_layout_clear(uint16_t pal) {
    if (pal == 0) {
        bitmap_rect_fill(&ui_bitmap, 0, 0, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS, BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
    } else {
        bitmap_rect_clear(&ui_bitmap, 0, 0, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS);
    }
    INIT_SCREEN_PATTERN(bitmap_screen2, pal, 0);
}

void ui_layout_bars(void) {
    bitmap_rect_fill(&ui_bitmap, 0, 8, WS_DISPLAY_WIDTH_PIXELS, WS_DISPLAY_HEIGHT_PIXELS - 16, BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
    INIT_SCREEN_PATTERN(bitmap_screen2, (iy == 0 || iy == 17) ? WS_SCREEN_ATTR_PALETTE(2) : 0, 0);
}
