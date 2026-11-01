/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <nilefs.h>
#include <ws/display.h>
#include <ws/system.h>
#include <wsx/planar_convert.h>
#include "errors.h"
#include "settings.h"
#include "ui/ui.h"
#include "util/file.h"
#include "util/input.h"
#include "util/bmp.h"
#include "util/math.h"

static void set_mono_shade_palette(uint8_t __far *palette, int color_count) {
    outportw(WS_SCR_PAL_0_PORT, 0x7654);
    uint16_t shades = 0;
    for (int i = 0; i < color_count; i++) {
        uint8_t b = *(palette++);
        uint8_t g = *(palette++);
        uint8_t r = *(palette++);
        palette++;
        shades |= (math_rgb8_to_greyscale(r, g, b) >> 4) << (i * 4);
    }
    outportb(WS_LCD_SHADE_01_PORT, 0xFF);
    outportw(WS_LCD_SHADE_45_PORT, shades ^ 0xFFFF);
}

static void center_viewport(uint8_t width, uint8_t height) {
    uint8_t xo = (WS_DISPLAY_WIDTH_PIXELS - width) >> 1;
    uint8_t yo = ((WS_DISPLAY_HEIGHT_PIXELS - height) >> 1);

    outportb(WS_SCR2_SCRL_X_PORT, -xo + inportb(WS_SCR2_SCRL_X_PORT));
    outportb(WS_SCR2_SCRL_Y_PORT, -yo + inportb(WS_SCR2_SCRL_Y_PORT));
    outportb(WS_SCR2_WIN_X1_PORT, xo);
    outportb(WS_SCR2_WIN_Y1_PORT, yo);
    outportb(WS_SCR2_WIN_X2_PORT, xo + width - 1);
    outportb(WS_SCR2_WIN_Y2_PORT, yo + height - 1);
}

int ui_bmpview(const char *path) {
    bmp_header_t __far* bmp = MK_FP(0x1000, 0x0000);
    int result = f_read_unbanked(bmp, path, 65535, NULL);
    if (result != FR_OK) {
        return result;
    }

    if (!bmp_is_header_valid(bmp) || bmp->width > WS_DISPLAY_WIDTH_PIXELS
        || bmp->height > WS_DISPLAY_HEIGHT_PIXELS || bmp->compression != 0) {
        return ERR_FILE_FORMAT_INVALID;
    }

    uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
    int color_count = bmp_calculate_color_count(bmp, data);
    uint16_t pitch = bmp_calculate_pitch_bytes(bmp);

    uint16_t bmp_loader_flags = 0;
    void *bmp_loader_dest = (void*) 0x2000;
    if (ws_system_is_color_active()) {
        if (color_count > 16) {
            return ERR_FILE_FORMAT_INVALID;
        }
        if (bmp->bpp >= 4) {
            bmp_loader_flags |= BMP_LOADER_FLAG_4BPP | BMP_LOADER_FLAG_PACKED;
            bmp_loader_dest = (void*) 0x4000;
        }
    }

    ui_hide();
    ui_hide_icons();

    // set screen
    ui_layout_clear_pattern();

    // configure palette
    uint8_t __far *palette = MK_FP(0x1000, 14 + bmp->header_size);
    if (ws_system_is_color_active()) {
        ws_system_set_mode(bmp->bpp >= 4 ? WS_MODE_COLOR_4BPP_PACKED : WS_MODE_COLOR);
        bmp_convert_palette_to_rgb4(WS_DISPLAY_COLOR_MEM(0), palette, color_count, bmp_loader_flags);
        WS_DISPLAY_COLOR_MEM(1)[0] = WS_DISPLAY_COLOR_MEM(0)[0];
    } else {
        set_mono_shade_palette(palette, color_count);
    }

    result = bmp_convert_data(bmp->width, bmp->height, pitch, bmp->bpp, bmp_loader_dest, data, bmp_loader_flags);
    if (result) goto bmp_convert_error;

    center_viewport(bmp->width, bmp->height);
    outportw(WS_DISPLAY_CTRL_PORT, (16 << 8) | WS_DISPLAY_CTRL_SCR2_ENABLE | WS_DISPLAY_CTRL_SCR2_WIN_INSIDE);
    input_wait_any_key();

bmp_convert_error:
    ui_init();
    settings_load();

    ui_layout_clear(0);

    return result;
}
