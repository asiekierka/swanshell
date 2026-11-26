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
#include <ws/timer.h>
#include "ui_selector.h"
#include "../main.h"
#include "cart/status.h"
#include "lang.h"
#include "lang_gen.h"
#include "settings.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "util/input.h"
#include "util/util.h"

#define SELECTOR_Y_OFFSET 8

static inline void ui_selector_set_active_font(ui_selector_config_t *config) {
    bitmapfont_set_active_font(config->style == UI_SELECTOR_STYLE_16 ? font16_bitmap : font8_bitmap);
}

#define UI_SELECTOR_ROW_CONFIG() \
    uint16_t row_count, row_height, row_offset; \
    if (config->style == UI_SELECTOR_STYLE_16) { \
        row_height = 16; \
        row_offset = 2; \
    } else { \
        row_height = 8; \
        row_offset = 0; \
    } \
    row_count = (screen_height - 16) / row_height;

void ui_selector_clear_selection(ui_selector_config_t *config) {
    ui_effect_select_clear();
}

#define UI_SELECTOR_OFFSET_TO_BOUNDS() \
    if (config->count > 0 && config->offset >= config->count) \
        config->offset = config->count - 1

#define SCROLL_TICKS_MIN 96
#define SCROLL_TICKS_PACE_SHIFT 5
#define SCROLL_TICKS_PACE_MASK ((1 << (SCROLL_TICKS_PACE_SHIFT)) - 1)

uint16_t ui_selector(ui_selector_config_t *config) {
    char sbuf[33];
    bool full_redraw = true;
    uint16_t prev_offset = 0xFFFF;
    uint16_t scroll_ticks = 0;

    UI_SELECTOR_ROW_CONFIG();

    if (config->count == 0) {
        ui_layout_bars();
        ui_selector_set_active_font(config);
        ui_draw_statusbar_lr(lang_keys[LK_UI_FILE_SELECTOR_EMPTY], config->info_str);

        while (true) {
            if (idle_until_vblank() || cart_status_apply_orientation_change())
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
            if (settings.file_flags & SETTING_FILE_SCROLL_LONG_NAMES) {
                if (scroll_ticks > SCROLL_TICKS_MIN) {
                    ui_selector_set_active_font(config);
                    int y_offset = (prev_offset % row_count) * row_height + SELECTOR_Y_OFFSET;
                    bitmap_rect_fill(&ui_bitmap, 0, y_offset + row_offset, screen_width, row_height, BITMAP_COLOR_2BPP(0));
                    config->draw(config, prev_offset, y_offset + row_offset, 0);
                }
                scroll_ticks = 0;
            }

            bool draw_filenames = prev_offset == 0xFFFF || ((prev_offset / row_count) != (config->offset / row_count));
            bool draw_highlights = prev_offset == 0xFFFF || ((prev_offset % row_count) != (config->offset % row_count));
            if (draw_filenames) {
                ui_selector_set_active_font(config);
                bitmap_rect_fill(&ui_bitmap, 0, SELECTOR_Y_OFFSET, screen_width, row_height * row_count, BITMAP_COLOR_4BPP(0));
                for (int i = 0; i < row_count; i++) {
                    uint16_t offset = ((config->offset / row_count) * row_count) + i;
                    if (offset >= config->count) break;

                    config->draw(config, offset, i * row_height + SELECTOR_Y_OFFSET + row_offset, 0);
                }

                snprintf(sbuf, sizeof(sbuf), lang_keys[LK_UI_FILE_SELECTOR_PAGE_FORMAT], (config->offset / row_count) + 1, ((config->count + row_count - 1) / row_count));
                ui_draw_statusbar_lr(sbuf, config->info_str);
            }
            if (draw_highlights) {
                uint16_t prev_sel = (prev_offset % row_count);
                uint16_t sel = (config->offset % row_count);
                if (full_redraw) {
                    for (int iy = 0; iy < row_count; iy++) {
                        ui_effect_select_tiles(0, iy*(row_height>>3) + 1, screen_width>>3, row_height>>3, iy == sel);
                    }
                } else {
                    uint16_t prev_sel_tile = prev_sel;
                    uint16_t sel_tile = sel;
                    if (row_height > 8) {
                        prev_sel_tile <<= 1;
                        sel_tile <<= 1;
                    }

                    ui_effect_select_tiles(0, prev_sel_tile + 1, screen_width>>3, row_height >> 3, false);
                    ui_effect_select_tiles(0, sel_tile + 1, screen_width>>3, row_height >> 3, true);
                }
            }

            prev_offset = config->offset;
            full_redraw = false;
        }

        if (idle_until_vblank() || cart_status_apply_orientation_change())
            return UI_SELECTOR_RELOAD_REQUESTED;

        if (settings.file_flags & SETTING_FILE_SCROLL_LONG_NAMES) {
            scroll_ticks++;
            if (scroll_ticks > SCROLL_TICKS_MIN && !(scroll_ticks & SCROLL_TICKS_PACE_MASK)) {
                ui_selector_set_active_font(config);
                int y_offset = (config->offset % row_count) * row_height + SELECTOR_Y_OFFSET;
                config->draw(config, config->offset, y_offset + row_offset, (scroll_ticks - SCROLL_TICKS_MIN) >> SCROLL_TICKS_PACE_SHIFT);
            }
        }

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
