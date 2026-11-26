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
#include <ws.h>
#include <ws/display.h>
#include "../../../build/menu/assets/menu/screen_patterns.h"
#include "bitmap_internal.h"
#include "ui.h"

#define UI_DO_ROTATE_XYWH_TILES(x,y,width,height) \
    if (!bitmap_rotation) { \
        uint16_t tmp = y; \
        y = x; \
        x = WS_DISPLAY_WIDTH_TILES - height - tmp; \
        \
        tmp = width; \
        width = height; \
        height = tmp; \
    }

void ui_effect_select_tiles(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool selected) {
    UI_DO_ROTATE_XYWH_TILES(x, y, width, height);

    uint16_t pal = selected ? WS_SCREEN_ATTR_PALETTE(1) : 0;
    if (ws_system_is_color_active()) {
        if (ui_has_wallpaper() && !selected) {
            ws_screen_put_tiles_ex(ui_screen_bg, ui_layout_wallpaper_data, x, y, 32, x, y, width, height);
        } else {
            ws_screen_fill_tiles(ui_screen_bg, 0x1FF | pal, x, y, width, height);
        }
    }
    ws_screen_modify_tiles(ui_screen_fg, ~WS_SCREEN_ATTR_PALETTE_MASK, pal, x, y, width, height);
}
