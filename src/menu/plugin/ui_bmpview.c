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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <nilefs.h>
#include <ws/system.h>
#include "errors.h"
#include "../ui/ui.h"
#include "../util/input.h"
#include "../main.h"

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

int ui_bmpview(const char *path) {
    FIL fp;
    uint8_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) {
        return result;
    }

    if (f_size(&fp) > 65535) {
        f_close(&fp);
        return ERR_FILE_TOO_LARGE;
    }

    result = f_read(&fp, MK_FP(0x1000, 0x0000), f_size(&fp), NULL);
    f_close(&fp);
    if (result != FR_OK) {
        return result;
    }

    bmp_header_t __far* bmp = MK_FP(0x1000, 0x0000);
    if (bmp->magic != 0x4d42 || bmp->header_size < 40 ||
        bmp->width <= 0 || bmp->width > WS_DISPLAY_WIDTH_PIXELS || bmp->height <= 0 || bmp->height > WS_DISPLAY_HEIGHT_PIXELS ||
        bmp->compression != 0) {
        return ERR_FILE_FORMAT_INVALID;
    }

    int color_count = bmp->color_count;
    uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
    uint16_t pitch = (((bmp->width * bmp->bpp) + 31) / 32) << 2;

    if (!color_count) {
        color_count = 1 << bmp->bpp;
    }
    if (bmp->bpp == 8 && color_count > 16) {
        color_count = 1;
        uint8_t __far *data_chk = data;
        for (uint16_t y = 0; y < bmp->height; y++) {
            for (uint16_t x = 0; x < bmp->width; x++, data_chk++) {
                color_count = MAX(color_count, *data_chk + 1);
            }
        }
    }
    if (ws_system_is_color_active()) {
        if (color_count > 16 || (bmp->bpp != 1 && bmp->bpp != 4 && bmp->bpp != 8)) {
            return ERR_FILE_FORMAT_INVALID;
        }
    } else {
        if (bmp->bpp != 1) {
            return ERR_FILE_FORMAT_INVALID;
        }
    }

    outportw(WS_DISPLAY_CTRL_PORT, 0);

    // set screen
    int ip = 0;
    for (int ix = 0; ix < 28; ix++) {
        for (int iy = 0; iy < 18; iy++) {
            ws_screen_put_tile(bitmap_screen2, (ip++), ix, iy);
        }
    }
    
    // configure palette
    uint8_t __far *palette = MK_FP(0x1000, 14 + bmp->header_size);
    if (ws_system_is_color_active()) {
        ws_system_set_mode(bmp->bpp >= 4 ? WS_MODE_COLOR_4BPP_PACKED : WS_MODE_COLOR);

        for (int i = 0; i < color_count; i++) {
            uint8_t b = *(palette++);
            uint8_t g = *(palette++);
            uint8_t r = *(palette++);
            palette++;
            WS_DISPLAY_COLOR_MEM(0)[i] = WS_RGB(r >> 4, g >> 4, b >> 4);
        }
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

    // copy data
    if (bmp->bpp == 8) {
        for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
            uint16_t __far *line_src = (uint16_t __far*) data;
            uint8_t *line_dst = (uint8_t*) (0x4000 + (((uint16_t)bmp->height - 1 - y) * 4));
            for (uint8_t x = 0; x < bmp->width; x += 8, line_dst += 18 * 32 - 4) {
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
    } else if (bmp->bpp == 4) {
        for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
            uint32_t __far *line_src = (uint32_t __far*) data;
            uint32_t *line_dst = (uint32_t*) (0x4000 + (((uint16_t)bmp->height - 1 - y) * 4));
            for (uint8_t x = 0; x < bmp->width; x += 8, line_src++, line_dst += 18 * 8) {
                *line_dst = *line_src;
            }
        }
    } else if (bmp->bpp == 1) {
        for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
            uint8_t __far *line_src = (uint8_t __far*) data;
            uint16_t *line_dst = (uint16_t*) (0x2000 + (((uint16_t)bmp->height - 1 - y) * 2));
            for (uint8_t x = 0; x < bmp->width; x += 8, line_src++, line_dst += 18 * 8) {
                *line_dst = *line_src;
            }
        }
    }

    uint8_t xo = (WS_DISPLAY_WIDTH_PIXELS - bmp->width) >> 1;
    uint8_t yo = ((WS_DISPLAY_HEIGHT_PIXELS - bmp->height) >> 1);

    outportb(WS_SCR2_SCRL_X_PORT, -xo);
    outportb(WS_SCR2_SCRL_Y_PORT, -yo + inportb(WS_SCR2_SCRL_Y_PORT));
    outportb(WS_SCR2_WIN_X1_PORT, xo);
    outportb(WS_SCR2_WIN_Y1_PORT, yo);
    outportb(WS_SCR2_WIN_X2_PORT, xo + bmp->width - 1);
    outportb(WS_SCR2_WIN_Y2_PORT, yo + bmp->height - 1);
    outportw(WS_DISPLAY_CTRL_PORT, (16 << 8) | WS_DISPLAY_CTRL_SCR2_ENABLE | WS_DISPLAY_CTRL_SCR2_WIN_INSIDE);
    input_wait_any_key();

    ui_init();
    ui_layout_clear(0);

    return 0;
}
