/**
 * Copyright (c) 2026 Adrian Siekierka
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
#include <ws.h>
#include <nilefs.h>
#include <ws/display.h>
#include <wsx/utf8.h>
#include "errors.h"
#include "lang.h"
#include "../ui/ui.h"
#include "../util/file.h"
#include "../util/input.h"
#include "../util/memops.h"
#include "../util/util.h"
#include "plugin.h"
#include "settings.h"
#include "ui/bitmap.h"

int ui_txtview(const char *path) {
    FIL fp;

    ui_layout_bars();

    uint8_t result = f_open(&fp, path, FA_READ);
    if (result != FR_OK) {
        return result;
    }

    ui_draw_titlebar_filename(path);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return ERR_FILE_TOO_LARGE;
    }

    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
    result = f_read_rom_banked(&fp, 0, size, NULL, NULL);
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);

    f_close(&fp);
    if (result != FR_OK) {
        return result;
    }
    
    ui_draw_statusbar(NULL);

    uint32_t file_start_pos = 0;
    uint32_t file_next_pos = 0;

    while (true) {
        ui_layout_bars();

        // Display text.
        bitmapfont_set_active_font(font16_bitmap);
        int y = 8;
        int x = 1;
        uint32_t file_pos = file_start_pos;
        file_next_pos = 0;
        while (file_pos < size) {
            uint32_t file_prev_pos = file_pos;
            ws_bank_rom0_set(file_pos >> 16);
            ws_bank_rom1_set(file_pos >> 16);
            const char __far *ptr = MK_FP(0x2000 | ((file_pos >> 4) & 0xFFF), file_pos & 0xF);
            uint32_t ch = wsx_utf8_decode_next(&ptr);
            file_pos = (file_pos & ~0xF) + FP_OFF(ptr);

            if (ch < 0x20) {
                if (ch == '\n') {
                    x = 1;
                    y += bitmapfont_get_font_height();
                    if (!file_next_pos) {
                        file_next_pos = file_pos;
                    }
                    if (y >= (WS_DISPLAY_HEIGHT_PIXELS - 8)) {
                        break;
                    }
                } else if (ch == '\t') {
                    x = (x + 0x21) & ~0x1F;
                }
                continue;
            }
            uint16_t chw = bitmapfont_get_char_width(ch);
            if ((x + chw) >= WS_DISPLAY_WIDTH_PIXELS) {
                x = 1;
                y += bitmapfont_get_font_height();
                if (!file_next_pos) {
                    file_next_pos = file_prev_pos;
                }
                if (y >= (WS_DISPLAY_HEIGHT_PIXELS - 8)) {
                    break;
                }
            }
            bitmapfont_draw_char(&ui_bitmap, x, y, ch);
            x += chw;
        }

        input_wait_any_key();
        if (input_pressed & (WS_KEY_B | WS_KEY_START)) {
            break;
        }
        if (file_pos >= size) {
            break;
        }
        file_start_pos = file_next_pos;
    }

    return 0;
}
