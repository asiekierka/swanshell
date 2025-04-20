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
#include "errors.h"
#include "ui.h"
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
        bmp->width <= 0 || bmp->width > 224 || bmp->height <= 0 || bmp->height > 144 ||
        bmp->compression != 0 || bmp->color_count > (ws_system_color_active() ? 16 : 4) ||
        (bmp->bpp != 1 && (!ws_system_color_active() || bmp->bpp != 4))) {
        return ERR_FILE_FORMAT_INVALID;
    }

    outportw(IO_DISPLAY_CTRL, 0);

    // set screen
    int ip = 0;
    for (int ix = 0; ix < 28; ix++) {
        for (int iy = 0; iy < 18; iy++) {
            ws_screen_put_tile(bitmap_screen2, (ip++), ix, iy);
        }
    }
    
    // configure palette
    uint8_t __far *palette = MK_FP(0x1000, 14 + bmp->header_size);
    if (ws_system_color_active()) {
        ws_mode_set(bmp->bpp == 4 ? WS_MODE_COLOR_4BPP_PACKED : WS_MODE_COLOR);

        for (int i = 0; i < bmp->color_count; i++) {
            uint8_t b = *(palette++);
            uint8_t g = *(palette++);
            uint8_t r = *(palette++);
            palette++;
            MEM_COLOR_PALETTE(0)[i] = RGB(r >> 4, g >> 4, b >> 4);
        }
        MEM_COLOR_PALETTE(1)[0] = MEM_COLOR_PALETTE(0)[0];
    } else {
        outportw(IO_SCR_PAL_0, 0x7654);
        uint16_t shades = 0;
        for (int i = 0; i < bmp->color_count; i++) {
            uint16_t b = *(palette++);
            uint16_t g = *(palette++);
            uint16_t r = *(palette++);
            palette++;
            shades |= ((77 * r + 150 * g + 29 * b) >> 12) << (i * 4);
        }
        outportb(IO_LCD_SHADE_01, 0xFF);
        outportw(IO_LCD_SHADE_45, shades ^ 0xFFFF);
    }

    // copy data
    uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
    uint16_t pitch = (((bmp->width * bmp->bpp) + 31) / 32) << 2;
    if (bmp->bpp == 4) {
        for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
            uint32_t __far *line_src = (uint32_t __far*) data;
            uint32_t *line_dst = (uint32_t*) (0x4000 + (((uint16_t)bmp->height - 1 - y) * 4));
            for (uint8_t x = 0; x < bmp->width; x += 8, line_src++, line_dst += 18 * 8) {
                *line_dst = *line_src;
            }
        }
    } else if (bmp->bpp == 1) {
        for (uint8_t y = 0; y < bmp->height; y++, data += pitch) {
            uint8_t __far *line_src = (uint32_t __far*) data;
            uint16_t *line_dst = (uint16_t*) (0x2000 + (((uint16_t)bmp->height - 1 - y) * 2));
            for (uint8_t x = 0; x < bmp->width; x += 8, line_src++, line_dst += 18 * 8) {
                *line_dst = *line_src;
            }
        }
    }

    uint8_t xo = (224 - bmp->width) >> 1;
    uint8_t yo = ((144 - bmp->height) >> 1);

    outportb(IO_SCR2_SCRL_X, -xo);
    outportb(IO_SCR2_SCRL_Y, -yo + inportb(IO_SCR2_SCRL_Y));
    outportb(IO_SCR2_WIN_X1, xo);
    outportb(IO_SCR2_WIN_Y1, yo);
    outportb(IO_SCR2_WIN_X2, xo + bmp->width - 1);
    outportb(IO_SCR2_WIN_Y2, yo + bmp->height - 1);
    outportw(IO_DISPLAY_CTRL, (16 << 8) | DISPLAY_SCR2_ENABLE | DISPLAY_SCR2_WIN_INSIDE);
    input_wait_any_key();

    ui_init();

    return 0;
}
