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
#include <ws.h>
#include <ws/display.h>
#include "bitmap.h"
#include "lang.h"
#include "ui/ui.h"
#include "ui_popup_list.h"
#include "../main.h"
#include "../util/input.h"
#include "../util/math.h"

#define LIST_ENTRY_X_PADDING 2
#define LIST_ENTRY_Y_OFFSET 1

int16_t ui_popup_list(ui_popup_list_config_t *config) {
    int option_count = 0;
    uint8_t option_width[UI_POPUP_LIST_MAX_OPTION_COUNT];
    uint16_t list_inner_width = 0;
    bitmapfont_set_active_font(font16_bitmap);
    for (; option_count < UI_POPUP_LIST_MAX_OPTION_COUNT; option_count++) {
        if (!config->option[option_count]) break;
        option_width[option_count] = bitmapfont_get_string_width(config->option[option_count], WS_DISPLAY_WIDTH_PIXELS);
        list_inner_width = MAX(list_inner_width, option_width[option_count]);
    }
    uint16_t list_width = list_inner_width + ((LIST_ENTRY_X_PADDING + 1) * 2);
    uint16_t list_height = bitmapfont_get_font_height() * option_count + 2;
    uint16_t list_x = (WS_DISPLAY_WIDTH_PIXELS - list_width) >> 1;
    uint16_t list_y = (WS_DISPLAY_HEIGHT_PIXELS - list_height) >> 1;

    bitmap_rect_draw(&ui_bitmap, list_x, list_y, list_width, list_height,
        BITMAP_COLOR(3, 3, BITMAP_COLOR_MODE_STORE), false);
    bitmap_rect_fill(&ui_bitmap, list_x + 1, list_y + 1, list_width - 2, list_height - 2,
        BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));

    for (int i = 0; i < option_count; i++) {
        bitmapfont_draw_string(&ui_bitmap,
            (WS_DISPLAY_WIDTH_PIXELS - option_width[i]) >> 1,
            list_y + 1 + LIST_ENTRY_Y_OFFSET + bitmapfont_get_font_height() * i,
            config->option[i], WS_DISPLAY_WIDTH_PIXELS);
    }

    uint8_t selected = 0;
    uint8_t prev_selected = 0xFF;

    wait_for_vblank();
    while (true) {
        if (selected != prev_selected) {
            if (prev_selected != 0xFF) {
                bitmap_rect_fill(&ui_bitmap, list_x + 1, list_y + 1 + bitmapfont_get_font_height() * prev_selected,
                    list_width - 2, bitmapfont_get_font_height(), BITMAP_COLOR(1, 1, BITMAP_COLOR_MODE_XOR));    
            }

            bitmap_rect_fill(&ui_bitmap, list_x + 1, list_y + 1 + bitmapfont_get_font_height() * selected,
                list_width - 2, bitmapfont_get_font_height(), BITMAP_COLOR(1, 1, BITMAP_COLOR_MODE_XOR));    
            prev_selected = selected;
        }

        wait_for_vblank();
        input_update();
        uint16_t keys_pressed = input_pressed;

        if (keys_pressed & KEY_UP) {
            if (selected > 0) {
                selected--;
            } else {
                selected = option_count - 1;
            }
        }
        if (keys_pressed & KEY_DOWN) {
            selected++;
            if (selected >= option_count) {
                selected = 0;
            }
        }
        if (keys_pressed & WS_KEY_A) {
            return selected;
        }
        if (keys_pressed & WS_KEY_B) {
            return UI_POPUP_ACTION_BACK;
        }
    }
}
