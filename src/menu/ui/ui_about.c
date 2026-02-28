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
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <ws/display.h>
#include "bitmap.h"
#include "errors.h"
#include "lang.h"
#include "lang_gen.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "util/input.h"
#include "util/util.h"
#include "ui_about.h"

static const char __far s_name_version[] = "%s " VERSION;

void ui_about(void) {
    char buf[129];

    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SETTINGS_ABOUT]);
    ui_draw_statusbar(NULL);

    int y = 23;
    sprintf(buf, s_name_version, lang_keys[LK_NAME]);
    bitmapfont_set_active_font(font16_bitmap);
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, 65535)) >> 1,
        y, buf, 65535);

    y += 14;
    strcpy(buf, lang_keys[LK_NAME_COPYRIGHT]);
    bitmapfont_set_active_font(font8_bitmap);
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, 65535)) >> 1,
        y, buf, 65535);

    y += 11;
    sprintf(buf, lang_keys[LK_ABOUT_RAM_BYTES_FREE], mem_query_free());
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, 65535)) >> 1,
        y, buf, 65535);

    y += 19;
    bitmapfont_draw_string_box(&ui_bitmap, 4, y, lang_keys[LK_NAME_COPYRIGHT_INFO], WS_DISPLAY_WIDTH_PIXELS - 8, 1, 0);

    input_wait_any_key();
}
