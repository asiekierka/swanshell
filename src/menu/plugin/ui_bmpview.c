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
#include "util/input.h"
#include "main.h"
#include "util/bmp.h"

int bmp_load(const char *path) {
    FIL fp;
    uint16_t br;
    uint8_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) {
        return result;
    }

    if (f_size(&fp) > 65535) {
        f_close(&fp);
        return ERR_FILE_TOO_LARGE;
    }

    result = f_read(&fp, MK_FP(0x1000, 0x0000), f_size(&fp), &br);
    f_close(&fp);
    return result;
}

int bmp_calculate_color_count(const bmp_header_t __far *bmp, const void __far *data) {
    int color_count = bmp->color_count;
    if (!color_count) {
        color_count = 1 << bmp->bpp;
    }
    if (bmp->bpp == 8 && color_count > 16) {
        color_count = 1;
        const uint8_t __far *data_chk = data;
        for (uint16_t y = 0; y < bmp->height; y++) {
            for (uint16_t x = 0; x < bmp->width; x++, data_chk++) {
                color_count = MAX(color_count, *data_chk + 1);
            }
        }
    }
    return color_count;
}

#define BMP_LOADER_FLAG_4BPP 0x0001
#define BMP_LOADER_FLAG_PACKED 0x0002

int bmp_convert_palette_to_rgb4(uint16_t *dest, const void __far *src, int color_count) {
    const uint8_t __far *palette = src;
    for (int i = 0; i < color_count; i++) {
        uint8_t b = *(palette++);
        uint8_t g = *(palette++);
        uint8_t r = *(palette++);
        palette++;
        *(dest++) = WS_RGB(r >> 4, g >> 4, b >> 4);
    }
}

int bmp_convert_data(uint16_t width, uint16_t height, uint16_t pitch, uint16_t bpp, void *dest, const void __far *src, uint16_t flags) {
    const uint8_t __far *src8 = src;
    if (bpp == 8) {
        if (!(flags & BMP_LOADER_FLAG_4BPP)) return ERR_FILE_FORMAT_INVALID;
        for (uint8_t y = 0; y < height; y++, src8 += pitch) {
            uint16_t __far *line_src = (uint16_t __far*) src8;
            uint8_t *line_dst = (uint8_t*) (((uintptr_t) dest) + ((height - 1 - y) * 4));
            for (uint8_t x = 0; x < width; x += 8, line_dst += WS_DISPLAY_HEIGHT_TILES * 32 - 4) {
                uint16_t s;
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
                s = *(line_src++);
                *(line_dst++) = (s << 4) | (s >> 8);
            }
        }
    } else if (bpp == 4) {
        if (!(flags & BMP_LOADER_FLAG_4BPP)) return ERR_FILE_FORMAT_INVALID;
        for (uint8_t y = 0; y < height; y++, src8 += pitch) {
            uint32_t __far *line_src = (uint32_t __far*) src8;
            uint32_t *line_dst = (uint32_t*) (((uintptr_t) dest) + ((height - 1 - y) * 4));
            for (uint8_t x = 0; x < width; x += 8, line_src++, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                *line_dst = *line_src;
            }
        }
    } else if (bpp == 1) {
        if (flags & BMP_LOADER_FLAG_4BPP) return ERR_FILE_FORMAT_INVALID;
        for (uint8_t y = 0; y < height; y++, src8 += pitch) {
            uint8_t __far *line_src = (uint8_t __far*) src8;
            uint16_t *line_dst = (uint16_t*) (((uintptr_t) dest) + ((height - 1 - y) * 2));
            for (uint8_t x = 0; x < width; x += 8, line_src++, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                *line_dst = *line_src;
            }
        }
    } else {
        return ERR_FILE_FORMAT_INVALID;
    }
    if ((flags & (BMP_LOADER_FLAG_4BPP | BMP_LOADER_FLAG_PACKED)) == BMP_LOADER_FLAG_4BPP) {
        for (uint8_t y = 0; y < height; y++) {
            uint32_t *line_dst = ((uint32_t*) dest) + y;
            for (uint8_t x = 0; x < width; x += 8, line_dst += WS_DISPLAY_HEIGHT_TILES * 8) {
                *line_dst = wsx_planar_convert_4bpp_packed_row(*line_dst);
            }
        }
    }
    return 0;
}

int ui_bmpview(const char *path) {
    int result = bmp_load(path);
    if (result != FR_OK) {
        return result;
    }

    bmp_header_t __far* bmp = MK_FP(0x1000, 0x0000);
    if (bmp->magic != BMP_MAGIC || bmp->header_size < BMP_MIN_HEADER_SIZE ||
        bmp->width <= 0 || bmp->width > WS_DISPLAY_WIDTH_PIXELS || bmp->height <= 0 || bmp->height > WS_DISPLAY_HEIGHT_PIXELS ||
        bmp->compression != 0) {
        return ERR_FILE_FORMAT_INVALID;
    }

    uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
    int color_count = bmp_calculate_color_count(bmp, data);
    uint16_t pitch = (((bmp->width * bmp->bpp) + 31) / 32) << 2;

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
        bmp_convert_palette_to_rgb4(WS_DISPLAY_COLOR_MEM(0), palette, color_count);
        WS_DISPLAY_COLOR_MEM(1)[0] = WS_DISPLAY_COLOR_MEM(0)[0];
    } else {
        outportw(WS_SCR_PAL_0_PORT, 0x7654);
        uint16_t shades = 0;
        for (int i = 0; i < color_count; i++) {
            uint16_t b = *(palette++);
            uint16_t g = *(palette++);
            uint16_t r = *(palette++);
            palette++;
            shades |= ((77 * r + 150 * g + 29 * b) >> 12) << (i * 4);
        }
        outportb(WS_LCD_SHADE_01_PORT, 0xFF);
        outportw(WS_LCD_SHADE_45_PORT, shades ^ 0xFFFF);
    }

    result = bmp_convert_data(bmp->width, bmp->height, pitch, bmp->bpp, bmp_loader_dest, data, bmp_loader_flags);
    if (result) goto bmp_convert_error;

    uint8_t xo = (WS_DISPLAY_WIDTH_PIXELS - bmp->width) >> 1;
    uint8_t yo = ((WS_DISPLAY_HEIGHT_PIXELS - bmp->height) >> 1);

    outportb(WS_SCR2_SCRL_X_PORT, -xo + inportb(WS_SCR2_SCRL_X_PORT));
    outportb(WS_SCR2_SCRL_Y_PORT, -yo + inportb(WS_SCR2_SCRL_Y_PORT));
    outportb(WS_SCR2_WIN_X1_PORT, xo);
    outportb(WS_SCR2_WIN_Y1_PORT, yo);
    outportb(WS_SCR2_WIN_X2_PORT, xo + bmp->width - 1);
    outportb(WS_SCR2_WIN_Y2_PORT, yo + bmp->height - 1);
    outportw(WS_DISPLAY_CTRL_PORT, (16 << 8) | WS_DISPLAY_CTRL_SCR2_ENABLE | WS_DISPLAY_CTRL_SCR2_WIN_INSIDE);
    input_wait_any_key();

bmp_convert_error:
    ui_init();
    settings_load();

    ui_layout_clear(0);

    return result;
}
