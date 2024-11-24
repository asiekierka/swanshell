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
#include "bitmap.h"
#include "launch/launch.h"
#include "strings.h"
#include "ui.h"
#include "ui_selector.h"
#include "../util/input.h"
#include "../main.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "lang.h"

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
#if 0
    if (0) {
#else
    if (ws_system_is_color()) {
#endif
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

void ui_layout_bars(void) {
    bitmap_rect_clear(&ui_bitmap, 0, 0, 224, 144);
    INIT_SCREEN_PATTERN(bitmap_screen2, (iy == 0 || iy == 17) ? SCR_ENTRY_PALETTE(2) : 0);
}
