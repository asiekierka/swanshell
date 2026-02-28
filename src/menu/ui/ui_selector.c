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
#include "ui_selector.h"
#include "../util/input.h"
#include "../main.h"
#include "lang.h"
#include "lang_gen.h"
#include "ui/ui.h"

#define SELECTOR_Y_OFFSET 8

static inline void ui_selector_set_active_font(ui_selector_config_t *config) {
    bitmapfont_set_active_font(config->style == UI_SELECTOR_STYLE_16 ? font16_bitmap : font8_bitmap);
}

#define UI_SELECTOR_ROW_CONFIG() \
    uint16_t row_count, row_height, row_offset; \
    if (config->style == UI_SELECTOR_STYLE_16) { \
        row_count = 8; \
        row_height = 16; \
        row_offset = 3; \
    } else { \
        row_count = 16; \
        row_height = 8; \
        row_offset = 0; \
    }

void ui_selector_clear_selection(ui_selector_config_t *config) {
    for (int ix = 0; ix < 28; ix++) {
        for (int iy = 0; iy < 16; iy++) {
            uint16_t pal = 0;
            ws_screen_put_tile(bitmap_screen2, pal | ((iy + 1) + (ix * 18)), ix, iy + 1);
        }
    }

    if (ui_has_wallpaper()) {
        ui_selector_set_active_font(config);
        UI_SELECTOR_ROW_CONFIG();

        if (config->offset < config->count) {
            uint16_t prev_sel = (config->offset % row_count);
            bitmap_rect_fill(&ui_bitmap, 0, prev_sel * row_height + SELECTOR_Y_OFFSET, 28 * 8, row_height, BITMAP_COLOR_4BPP(0));
            config->draw(config, config->offset, prev_sel * row_height + SELECTOR_Y_OFFSET + row_offset);
        }
    }
}

#define UI_SELECTOR_OFFSET_TO_BOUNDS() \
    if (config->count > 0 && config->offset >= config->count) \
        config->offset = config->count - 1

uint16_t ui_selector(ui_selector_config_t *config) {
    char sbuf[33];
    bool full_redraw = true;
    uint16_t prev_offset = 0xFFFF;

    UI_SELECTOR_ROW_CONFIG();

    if (config->count == 0) {
        ui_layout_bars();
        ui_selector_set_active_font(config);
        ui_draw_statusbar(lang_keys[LK_UI_FILE_SELECTOR_EMPTY]);

        while (true) {
            if (idle_until_vblank())
                return UI_SELECTOR_RELOAD_REQUESTED;
            input_update();
            uint16_t keys_pressed = input_pressed;

            // User actions
            if (keys_pressed & config->key_mask)
                return keys_pressed & config->key_mask;
        }
    }

    while (true) {
        if (prev_offset != config->offset) {
            bool draw_filenames = prev_offset == 0xFFFF || ((prev_offset / row_count) != (config->offset / row_count));
            bool draw_highlights = prev_offset == 0xFFFF || ((prev_offset % row_count) != (config->offset % row_count));
            if (draw_filenames) {
                ui_selector_set_active_font(config);
                bitmap_rect_fill(&ui_bitmap, 0, SELECTOR_Y_OFFSET, 28 * 8, row_height * row_count, BITMAP_COLOR_4BPP(ui_has_wallpaper() ? 0 : 2));
                for (int i = 0; i < row_count; i++) {
                    uint16_t offset = ((config->offset / row_count) * row_count) + i;
                    if (offset >= config->count) break;

                    config->draw(config, offset, i * row_height + SELECTOR_Y_OFFSET + row_offset);
                }

                snprintf(sbuf, sizeof(sbuf), lang_keys[LK_UI_FILE_SELECTOR_PAGE_FORMAT], (config->offset / row_count) + 1, ((config->count + row_count - 1) / row_count));
                ui_draw_statusbar(sbuf);
                if (config->info_key)
                    ui_draw_statusbar_right(lang_keys[config->info_key]);
                draw_highlights |= ui_has_wallpaper();
            }
            if (draw_highlights) {
                uint16_t prev_sel = (prev_offset % row_count);
                uint16_t sel = (config->offset % row_count);
                if (full_redraw) {
                    for (int ix = 0; ix < 28; ix++) {
                        for (int iy = 0; iy < 16; iy++) {
                            uint16_t pal = 0;
                            if ((iy >> (row_height > 8 ? 1 : 0)) == sel) pal = WS_SCREEN_ATTR_PALETTE(1);
                            ws_screen_put_tile(bitmap_screen2, pal | ((iy + 1) + (ix * 18)), ix, iy + 1);
                        }
                    }
                } else {
                    uint16_t prev_sel_tile = prev_sel;
                    uint16_t sel_tile = sel;
                    if (row_height > 8) {
                        prev_sel_tile <<= 1;
                        sel_tile <<= 1;
                    }
                    for (int ix = 0; ix < 28; ix++) {
                        ws_screen_put_tile(bitmap_screen2, ((prev_sel_tile + 1) + (ix * 18)), ix, prev_sel_tile + 1);
                        ws_screen_put_tile(bitmap_screen2, WS_SCREEN_ATTR_PALETTE(1) | ((sel_tile + 1) + (ix * 18)), ix, sel_tile + 1);

                        if (row_height > 8) {
                            ws_screen_put_tile(bitmap_screen2, ((prev_sel_tile + 2) + (ix * 18)), ix, prev_sel_tile + 2);
                            ws_screen_put_tile(bitmap_screen2, WS_SCREEN_ATTR_PALETTE(1) | ((sel_tile + 2) + (ix * 18)), ix, sel_tile + 2);
                        }
                    }
                }

                if (ui_has_wallpaper()) {
                    ui_selector_set_active_font(config);
                    // Redraw previous and current selected filename
                    if (!draw_filenames && prev_offset < config->count) {
                        bitmap_rect_fill(&ui_bitmap, 0, prev_sel * row_height + SELECTOR_Y_OFFSET, 28 * 8, row_height, BITMAP_COLOR_4BPP(0));
                        config->draw(config, prev_offset, prev_sel * row_height + SELECTOR_Y_OFFSET + row_offset);
                    }
                    if (config->offset < config->count) {
                        bitmap_rect_fill(&ui_bitmap, 0, sel * row_height + SELECTOR_Y_OFFSET, 28 * 8, row_height, BITMAP_COLOR_4BPP(2));
                        config->draw(config, config->offset, sel * row_height + SELECTOR_Y_OFFSET + row_offset);
                    }
                }
            }

            prev_offset = config->offset;
            full_redraw = false;
        }

        if (idle_until_vblank())
            return UI_SELECTOR_RELOAD_REQUESTED;
    	input_update();
        uint16_t keys_pressed = input_pressed;

        // Page/entry movement
        if (keys_pressed & WS_KEY_X1) {
            do {
                if (config->offset == 0 || ((config->offset - 1) % row_count) == (row_count - 1))
                    config->offset = config->offset + (row_count - 1);
                else
                    config->offset = config->offset - 1;
                UI_SELECTOR_OFFSET_TO_BOUNDS();
            } while (config->can_select != NULL && !config->can_select(config, config->offset));
        }
        if (keys_pressed & WS_KEY_X3) {
            do {
                if ((config->offset + 1) == config->count)
                    config->offset = config->offset - (config->offset % row_count);
                else if (((config->offset + 1) % row_count) == 0)
                    config->offset = config->offset - (row_count - 1);
                else
                    config->offset = config->offset + 1;
                UI_SELECTOR_OFFSET_TO_BOUNDS();
            } while (config->can_select != NULL && !config->can_select(config, config->offset));
        }
        if (keys_pressed & WS_KEY_X2) {
            do {
                if ((config->offset - (config->offset % row_count) + row_count) < config->count)
                    config->offset += row_count;
                UI_SELECTOR_OFFSET_TO_BOUNDS();
            } while (config->can_select != NULL && !config->can_select(config, config->offset));
        }
        if (keys_pressed & WS_KEY_X4) {
            do {
                if (config->offset >= row_count)
                    config->offset -= row_count;
                UI_SELECTOR_OFFSET_TO_BOUNDS();
            } while (config->can_select != NULL && !config->can_select(config, config->offset));
        }
        UI_SELECTOR_OFFSET_TO_BOUNDS();

        // User actions
        if (keys_pressed & config->key_mask)
            return keys_pressed & config->key_mask;
    }
}
