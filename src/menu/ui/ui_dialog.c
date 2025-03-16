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
#include "ui_dialog.h"
#include "../util/math.h"
#include "ui/ui.h"

#define ROUND_TO_8_WITH_BORDER(a) (((a)+23)&(~7))
#define INNER_GAP 4
#define APPEND_GAP(a) ((a) = (a) + ((a)?INNER_GAP:0))
#define APPEND_WITH_GAP(a,b) ((a) = (a) + ((a)?INNER_GAP:0) + (b))
#define CENTERED_IN_BOX(xofs,width,inner_width) ((xofs) + (((width) - (inner_width)) >> 1))

void ui_dialog_reset(ui_dialog_config_t *config) {
    config->width = 0;
    config->height = 0;
}

void ui_dialog_clear(ui_dialog_config_t *config) {
    if (config->width && config->height) {
        bitmap_rect_fill(&ui_bitmap, config->x, config->y, config->width, config->height,
            BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
    }
}

void ui_dialog_draw(ui_dialog_config_t *config) {
    uint16_t title_box_width;
    uint16_t title_box_height;
    uint16_t desc_box_width;
    uint16_t desc_box_height;

    uint16_t inner_width = 0;
    uint16_t inner_height = 0;

    if (config->width && config->height) {
        title_box_width = config->width;
        desc_box_width = config->width;
    } else {
        title_box_width = 192;
        desc_box_width = 192;
    }
    title_box_height = 0;
    desc_box_height = 0;

    if (config->title) {
        bitmapfont_set_active_font(font16_bitmap);
        bitmapfont_get_string_box(config->title, &title_box_width, &title_box_height);
        APPEND_WITH_GAP(inner_height, title_box_height);
        inner_width = MAX(inner_width, title_box_width);
    } else {
        title_box_width = 0;
    }
    if (config->description) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_get_string_box(config->description, &desc_box_width, &desc_box_height);
        APPEND_WITH_GAP(inner_height, desc_box_height);
        inner_width = MAX(inner_width, desc_box_width);
    } else {
        desc_box_width = 0;
    }
    if (config->progress_max) {
        APPEND_WITH_GAP(inner_height, 1);
        inner_width = MAX(inner_width, 64);
    }
    
    if (!config->width || !config->height) {
        config->width = ROUND_TO_8_WITH_BORDER(inner_width);
        config->height = ROUND_TO_8_WITH_BORDER(inner_height);
        config->x = (DISPLAY_WIDTH_PX - config->width) >> 1;
        config->y = (DISPLAY_HEIGHT_PX - config->height) >> 1;
    }

    uint16_t inner_y = CENTERED_IN_BOX(config->y, config->height, inner_height);
    inner_height = 0;

    ui_dialog_clear(config);
    bitmap_rect_draw(&ui_bitmap, config->x, config->y, config->width, config->height,
        BITMAP_COLOR(MAINPAL_COLOR_BLACK, 3, BITMAP_COLOR_MODE_STORE), false);

    if (config->title) {
        bitmapfont_set_active_font(font16_bitmap);
        bitmapfont_draw_string_box(&ui_bitmap,
            CENTERED_IN_BOX(config->x, config->width, title_box_width),
            inner_y + inner_height,
            config->title, title_box_width);
        APPEND_WITH_GAP(inner_height, title_box_height);
    }
    if (config->description) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string_box(&ui_bitmap,
            CENTERED_IN_BOX(config->x, config->width, desc_box_width),
            inner_y + inner_height,
            config->description, desc_box_width);
        APPEND_WITH_GAP(inner_height, desc_box_height);
    }
    if (config->progress_max) {
        APPEND_WITH_GAP(inner_height, 1);
        config->progress_y = inner_y + inner_height - 1;
    }

    ui_dialog_draw_update(config);
}

void ui_dialog_draw_update(ui_dialog_config_t *config) {
    if (config->progress_max && config->progress_step) {
        uint16_t p_width = config->width - 16;
        uint16_t p = config->progress_step * (uint32_t)p_width / config->progress_max;
        bitmap_rect_fill(&ui_bitmap, config->x + 8, config->progress_y, p, 1,
            BITMAP_COLOR(MAINPAL_COLOR_BLACK, 3, BITMAP_COLOR_MODE_STORE));
    }
}
