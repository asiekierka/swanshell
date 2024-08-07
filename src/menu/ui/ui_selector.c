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
#include <ws.h>
#include <ws/display.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include "ui_selector.h"
#include "../util/input.h"
#include "../main.h"
#include "../../../build/menu/assets/menu/lang.h"

#define SELECTOR_Y_OFFSET 8

uint16_t ui_selector(ui_selector_config_t *config) {
    char sbuf[33];
    bool full_redraw = true;
    uint16_t prev_offset = 0xFFFF;

    uint16_t row_count, row_height, row_offset;
    if (config->style == UI_SELECTOR_STYLE_16) {
        row_count = 8;
        row_height = 16;
        row_offset = 3;
    } else {
        row_count = 16;
        row_height = 8;
        row_offset = 0;
    }

    while (true) {
        if (prev_offset != config->offset) {
            if ((prev_offset / row_count) != (config->offset / row_count)) {
                bitmap_rect_fill(&ui_bitmap, 0, SELECTOR_Y_OFFSET, 28 * 8, row_height * row_count, BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
                // Draw filenames
                bitmapfont_set_active_font(config->style == UI_SELECTOR_STYLE_16 ? font16_bitmap : font8_bitmap);
                for (int i = 0; i < row_count; i++) {
                    uint16_t offset = ((config->offset / row_count) * row_count) + i;
                    if (offset >= config->count) break;

                    config->draw(config, offset, i * row_height + SELECTOR_Y_OFFSET + row_offset);
                }

                snprintf(sbuf, sizeof(sbuf), lang_keys_en[LK_UI_FILE_SELECTOR_PAGE_FORMAT], (config->offset / row_count) + 1, ((config->count + row_count - 1) / row_count));
                ui_draw_statusbar(sbuf);
            }
            if ((prev_offset % row_count) != (config->offset % row_count)) {
                // Draw highlights
#if 0
                if (full_redraw) {
                    for (int iy = 0; iy < row_count; iy++) {
                        uint16_t pal = 0;
                        if (iy == (config->offset % row_count)) pal = 2;
                        bitmap_rect_fill(&ui_bitmap, 0, iy * row_height + SELECTOR_Y_OFFSET, 224, row_height, BITMAP_COLOR(pal, 2, BITMAP_COLOR_MODE_STORE));
                    }
                } else {
                    int iy1 = (prev_file_offset % row_count);
                    int iy2 = (config->offset % row_count);
                    bitmap_rect_fill(&ui_bitmap, 0, iy1 * row_height + SELECTOR_Y_OFFSET, 224, row_height, BITMAP_COLOR(0, 2, BITMAP_COLOR_MODE_STORE));
                    bitmap_rect_fill(&ui_bitmap, 0, iy2 * row_height + SELECTOR_Y_OFFSET, 224, row_height, BITMAP_COLOR(2, 2, BITMAP_COLOR_MODE_STORE));
                }
#else
                uint16_t sel = (config->offset % row_count);
                if (full_redraw) {
                    for (int ix = 0; ix < 28; ix++) {
                        for (int iy = 0; iy < 16; iy++) {
                            uint16_t pal = 0;
                            if ((iy >> (row_height > 8 ? 1 : 0)) == sel) pal = SCR_ATTR_PALETTE(1);
                            ws_screen_put_tile(bitmap_screen2, pal | ((iy + 1) + (ix * 18)), ix, iy + 1);
                        }
                    }
                } else {
                    uint16_t prev_sel = (prev_offset % row_count);
                    if (row_height > 8) {
                        prev_sel <<= 1;
                        sel <<= 1;
                    }
                    for (int ix = 0; ix < 28; ix++) {
                        ws_screen_put_tile(bitmap_screen2, ((prev_sel + 1) + (ix * 18)), ix, prev_sel + 1);
                        ws_screen_put_tile(bitmap_screen2, SCR_ATTR_PALETTE(1) | ((sel + 1) + (ix * 18)), ix, sel + 1);
                        
                        if (row_height > 8) {
                            ws_screen_put_tile(bitmap_screen2, ((prev_sel + 2) + (ix * 18)), ix, prev_sel + 2);
                            ws_screen_put_tile(bitmap_screen2, SCR_ATTR_PALETTE(1) | ((sel + 2) + (ix * 18)), ix, sel + 2);
                        }
                    }
                }
#endif
            }

            prev_offset = config->offset;
            full_redraw = false;
        }

        wait_for_vblank();
    	input_update();
        uint16_t keys_pressed = input_pressed;

        // Page/entry movement
        if (keys_pressed & KEY_X1) {
            if (config->offset == 0 || ((config->offset - 1) % row_count) == (row_count - 1))
                config->offset = config->offset + (row_count - 1);
            else
                config->offset = config->offset - 1;
        }
        if (keys_pressed & KEY_X3) {
            if ((config->offset + 1) == config->count)
                config->offset = config->offset - (config->offset % row_count);
            else if (((config->offset + 1) % row_count) == 0)
                config->offset = config->offset - (row_count - 1);
            else
                config->offset = config->offset + 1;
        }
        if (keys_pressed & KEY_X2) {
            if ((config->offset - (config->offset % row_count) + row_count) < config->count)
                config->offset += row_count;
        }
        if (keys_pressed & KEY_X4) {
            if (config->offset >= row_count)
                config->offset -= row_count;
        }
        if (config->count > 0 && config->offset >= config->count)
            config->offset = config->count - 1;

        // User actions
        if (keys_pressed & config->key_mask)
            return keys_pressed & config->key_mask;
    }
}
