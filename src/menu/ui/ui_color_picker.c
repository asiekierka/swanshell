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

#include <stdbool.h>
#include <string.h>
#include <ws/display.h>
#include <ws/system.h>
#include "ui_color_picker.h"
#include "lang.h"
#include "lang_gen.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "main.h"
#include "util/input.h"

#define COLORBAR_WIDTH (WS_DISPLAY_WIDTH_PIXELS - 32)
#define COLORBAR_X ((WS_DISPLAY_WIDTH_PIXELS - COLORBAR_WIDTH) >> 1)
#define COLORBAR_ENTRY_WIDTH (COLORBAR_WIDTH / 16)
#define COLORBAR_ENTRY_HEIGHT 16
_Static_assert(((COLORBAR_ENTRY_WIDTH * 8) & 7) == 0, "Incompatible colorbar entry width");

#define COLORBAR_SEL_WIDTH 6
#define COLORBAR_SEL_HEIGHT 6
#define COLORBAR_SEL_X ((COLORBAR_ENTRY_WIDTH - COLORBAR_SEL_WIDTH) >> 1)
#define COLORBAR_SEL_Y ((COLORBAR_ENTRY_HEIGHT - COLORBAR_SEL_HEIGHT) >> 1)

static void draw_colorbar(uint8_t y, uint8_t start_palette, uint16_t rgb, uint8_t rgb_shift, uint16_t lang_key, bool full_redraw, bool masked) {
    uint16_t rgb_1 = 0x1 << rgb_shift;
    uint16_t rgb_mask = 0xF << rgb_shift;

    // Set palette entries
    for (int i = 0; i < 16; i++) {
        uint16_t local_rgb = rgb_1 * i;
        if (masked)
            local_rgb |= (rgb & ~rgb_mask);
        WS_DISPLAY_COLOR_MEM(start_palette + (i >> 3))[8 + (i & 7)] = local_rgb;
    }

    // Set rectangle palettes
    if (full_redraw) {
        ws_screen_modify_tiles(bitmap_screen2,
            ~WS_SCREEN_ATTR_PALETTE(0xF),
            WS_SCREEN_ATTR_PALETTE(start_palette),
            COLORBAR_X >> 3, y >> 3,
            COLORBAR_ENTRY_WIDTH, COLORBAR_ENTRY_HEIGHT >> 3);
        ws_screen_modify_tiles(bitmap_screen2,
            ~WS_SCREEN_ATTR_PALETTE(0xF),
            WS_SCREEN_ATTR_PALETTE(start_palette + 1),
            (COLORBAR_X >> 3) + COLORBAR_ENTRY_WIDTH, y >> 3,
            COLORBAR_ENTRY_WIDTH, COLORBAR_ENTRY_HEIGHT >> 3);
        }

    // Draw rectangles
    for (int i = 0; i < 16; i++) {
        bitmap_rect_fill(&ui_bitmap, COLORBAR_X + i * COLORBAR_ENTRY_WIDTH, y, COLORBAR_ENTRY_WIDTH, COLORBAR_ENTRY_HEIGHT,
            BITMAP_COLOR_4BPP(8 + (i & 7)));
    }
    if (full_redraw) {
        bitmap_rect_draw(&ui_bitmap, COLORBAR_X - 1, y - 1, COLORBAR_WIDTH + 2, COLORBAR_ENTRY_HEIGHT + 2, BITMAP_COLOR_2BPP(3), false);
        bitmapfont_draw_string(&ui_bitmap, COLORBAR_X - 1, y - 10, lang_keys[lang_key], COLORBAR_WIDTH + 2);
    }

    // Draw selected color
    int rgb_sel_ofs = (rgb >> rgb_shift) & 0xF;
    bitmap_rect_fill(&ui_bitmap, COLORBAR_X + rgb_sel_ofs * COLORBAR_ENTRY_WIDTH + COLORBAR_SEL_X + 1, y + COLORBAR_SEL_Y + 1,
         COLORBAR_SEL_WIDTH - 2, COLORBAR_SEL_HEIGHT - 2,
        BITMAP_COLOR_4BPP(7));
    bitmap_rect_draw(&ui_bitmap, COLORBAR_X + rgb_sel_ofs * COLORBAR_ENTRY_WIDTH + COLORBAR_SEL_X, y + COLORBAR_SEL_Y,
         COLORBAR_SEL_WIDTH, COLORBAR_SEL_HEIGHT,
        BITMAP_COLOR_4BPP(6), true);
}

#define COLOR_PREVIEW_WIDTH 16
#define COLOR_PREVIEW_HEIGHT 16
#define COLOR_PREVIEW_X (COLORBAR_X + COLORBAR_ENTRY_WIDTH*16 - COLOR_PREVIEW_WIDTH)
#define COLOR_PREVIEW_Y 16

void ui_color_picker(uint16_t *rgb) {
    uint16_t new_rgb = *rgb;

    // TODO: Mono model support
    if (!ws_system_is_color_active())
        return;

    bitmapfont_set_active_font(font8_bitmap);
    ui_layout_bars();
    ui_draw_statusbar(NULL);

    // Draw preview
    bitmap_rect_fill(&ui_bitmap, COLOR_PREVIEW_X, COLOR_PREVIEW_Y, COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT, BITMAP_COLOR_4BPP(5));
    ws_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(10),
        COLOR_PREVIEW_X >> 3, COLOR_PREVIEW_Y >> 3,
        COLOR_PREVIEW_WIDTH >> 3, COLOR_PREVIEW_HEIGHT >> 3);
    bitmap_rect_draw(&ui_bitmap, COLOR_PREVIEW_X - 1, COLOR_PREVIEW_Y - 1, COLOR_PREVIEW_WIDTH + 2, COLOR_PREVIEW_HEIGHT + 2, BITMAP_COLOR_2BPP(3), false);
    bitmapfont_draw_string(&ui_bitmap, COLOR_PREVIEW_X + COLOR_PREVIEW_WIDTH + 1 - bitmapfont_get_string_width(lang_keys[LK_COLOR_PREVIEW], 255), COLOR_PREVIEW_Y + COLOR_PREVIEW_HEIGHT + 2, lang_keys[LK_COLOR_PREVIEW], 255);

    int redraw_colorbars = 7;
    bool full_redraw_colorbars = true;
    bool colorbars_masked = true;
    int selector_offset = 0;

    while (true) {
        // Set selector colors
        WS_DISPLAY_COLOR_MEM(10)[5] = new_rgb;

        for (int i = 0; i < 3; i++) {
            uint16_t outer = selector_offset == i ? 0xFFF : 0x000;
            uint16_t inner = selector_offset == i ? 0x000 : 0xFFF;
            int o = 10 + i * 2;
            WS_DISPLAY_COLOR_MEM(o)[6] = outer;
            WS_DISPLAY_COLOR_MEM(o)[7] = inner;
            WS_DISPLAY_COLOR_MEM(o+1)[6] = outer;
            WS_DISPLAY_COLOR_MEM(o+1)[7] = inner;
        }

        // Draw color bars
        if (redraw_colorbars) {
            if (redraw_colorbars & 1)
                draw_colorbar(48, 10, new_rgb, 8, LK_COLOR_RED, full_redraw_colorbars, colorbars_masked);
            if (redraw_colorbars & 2)
                draw_colorbar(80, 12, new_rgb, 4, LK_COLOR_GREEN, full_redraw_colorbars, colorbars_masked);
            if (redraw_colorbars & 4)
                draw_colorbar(112, 14, new_rgb, 0, LK_COLOR_BLUE, full_redraw_colorbars, colorbars_masked);

            redraw_colorbars = 0;
            full_redraw_colorbars = false;
        }

        int component_offset = (8 - (selector_offset * 4));
        int component = (new_rgb >> component_offset) & 0xF;
        int redraw_local = colorbars_masked ? 7 : (1 << selector_offset);

        // Fetch input
        idle_until_vblank();
        input_update();

        if (input_pressed & WS_KEY_B)
            break;
        if (input_pressed & WS_KEY_A) {
            *rgb = new_rgb;
            break;
        }
        if (input_pressed & WS_KEY_X4) {
            // move -1
            if (component > 0) {
                component--;
                redraw_colorbars = redraw_local;
            }
        }
        if (input_pressed & WS_KEY_X2) {
            // move +1
            if (component < 15) {
                component++;
                redraw_colorbars = redraw_local;
            }
        }
        if (input_pressed & WS_KEY_X1) {
            if (selector_offset > 0)
                selector_offset--;
        }
        if (input_pressed & WS_KEY_X3) {
            if (selector_offset < 2)
                selector_offset++;
        }
        if (input_pressed & WS_KEY_Y4) {
            component = 0;
            redraw_colorbars = redraw_local;
        }
        if (input_pressed & WS_KEY_Y2) {
            component = 15;
            redraw_colorbars = redraw_local;
        }
        if (input_pressed & WS_KEY_Y1) {
            colorbars_masked = !colorbars_masked;
            redraw_colorbars = 7;
        }

        new_rgb = (new_rgb & ~(0xF << component_offset)) | (component << component_offset);
    }

    bitmap_rect_fill(&ui_bitmap, COLORBAR_X, 8, COLORBAR_WIDTH, WS_DISPLAY_HEIGHT_PIXELS - 16, BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
}