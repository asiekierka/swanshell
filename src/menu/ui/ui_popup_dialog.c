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
#include "ui_popup_dialog.h"
#include "../main.h"
#include "../util/input.h"
#include "../util/math.h"

#define INNER_GAP 4
#define MIN_PROGRESS_BAR_WIDTH 64
#define BUTTON_X_BORDER 3
#define BUTTON_Y_BORDER 2

#define ROUND_TO_8_WITH_BORDER(a) (((a)+23)&(~7))
#define APPEND_GAP(a) ((a) = (a) + ((a)?INNER_GAP:0))
#define APPEND_WITH_GAP(a,b) ((a) = (a) + ((a)?INNER_GAP:0) + (b))
#define APPEND_INCLUDING_GAP(a,b) ((a) = (a) + (INNER_GAP) + (b))

void ui_popup_dialog_reset(ui_popup_dialog_config_t *config) {
    config->width = 0;
    config->height = 0;
}

void ui_popup_dialog_clear(ui_popup_dialog_config_t *config) {
    if (config->width && config->height) {
        bitmap_rect_fill(&ui_bitmap, config->x, config->y, config->width, config->height,
            BITMAP_COLOR(2, 15, BITMAP_COLOR_MODE_STORE));
    }
}

// -1 = full redraw
static inline void ui_popup_dialog_draw_buttons(ui_popup_dialog_config_t *config, int16_t selected) {
    uint16_t button_x[UI_POPUP_DIALOG_MAX_BUTTON_COUNT];
    uint16_t button_w[UI_POPUP_DIALOG_MAX_BUTTON_COUNT];
    uint16_t button_width = 0;

    bitmapfont_set_active_font(font16_bitmap);
    uint16_t button_height = bitmapfont_get_font_height() - 1 + (BUTTON_Y_BORDER * 2);

    for (int i = 0; i < UI_POPUP_DIALOG_MAX_BUTTON_COUNT; i++) {
        if (!config->buttons[i]) break;
        if (i > 0) button_width += INNER_GAP;
        button_x[i] = button_width;
        button_w[i] = (BUTTON_X_BORDER * 2) + bitmapfont_get_string_width(lang_keys[config->buttons[i]], 224);
        button_width += button_w[i];
    }

    uint16_t xofs = UI_CENTERED_IN_BOX(config->x, config->width, button_width);
    if (selected == -1) {
        for (int i = 0; i < UI_POPUP_DIALOG_MAX_BUTTON_COUNT; i++) {
            if (!config->buttons[i]) break;
            bitmap_rect_draw(&ui_bitmap, xofs + button_x[i], config->buttons_y,
                button_w[i], button_height, BITMAP_COLOR(3, 3, BITMAP_COLOR_MODE_STORE), true);
        }
    }

    // TODO: partial refreshes
    for (int i = 0; i < UI_POPUP_DIALOG_MAX_BUTTON_COUNT; i++) {
        if (!config->buttons[i]) break;
        bitmap_rect_fill(&ui_bitmap, xofs + button_x[i] + 1, config->buttons_y + 1,
            button_w[i] - 2, button_height - 2, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));
        bitmapfont_draw_string(&ui_bitmap, xofs + button_x[i] + BUTTON_X_BORDER, config->buttons_y + BUTTON_Y_BORDER,
            lang_keys[config->buttons[i]], 224);  
        if (selected == i) {
            bitmap_rect_fill(&ui_bitmap, xofs + button_x[i] + 1, config->buttons_y + 1,
                button_w[i] - 2, button_height - 2, BITMAP_COLOR(1, 1, BITMAP_COLOR_MODE_XOR));    
        }  
    }
}

void ui_popup_dialog_draw(ui_popup_dialog_config_t *config) {
    uint16_t title_box_width;
    uint16_t title_box_height;
    uint16_t desc_box_width;
    uint16_t desc_box_height;

    uint16_t button_width = 0;
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
        inner_width = MAX(inner_width, MIN_PROGRESS_BAR_WIDTH);
    }
    if (config->buttons[0]) {
        bitmapfont_set_active_font(font16_bitmap);

        for (int i = 0; i < UI_POPUP_DIALOG_MAX_BUTTON_COUNT; i++) {
            if (!config->buttons[i]) break;
            if (i > 0) button_width += INNER_GAP;
            button_width += (BUTTON_X_BORDER * 2) + bitmapfont_get_string_width(lang_keys[config->buttons[i]], 224);
        }

        APPEND_WITH_GAP(inner_height, bitmapfont_get_font_height() - 1 + (BUTTON_Y_BORDER * 2));
        inner_width = MAX(inner_width, button_width);
    }
    
    if (!config->width || !config->height) {
        config->width = ROUND_TO_8_WITH_BORDER(inner_width);
        config->height = ROUND_TO_8_WITH_BORDER(inner_height);
        config->x = (DISPLAY_WIDTH_PX - config->width) >> 1;
        config->y = (DISPLAY_HEIGHT_PX - config->height) >> 1;
    }

    uint16_t inner_y = UI_CENTERED_IN_BOX(config->y, config->height, inner_height);
    inner_height = 0;

    ui_popup_dialog_clear(config);
    bitmap_rect_draw(&ui_bitmap, config->x, config->y, config->width, config->height,
        BITMAP_COLOR(MAINPAL_COLOR_BLACK, 3, BITMAP_COLOR_MODE_STORE), false);

    if (config->title) {
        bitmapfont_set_active_font(font16_bitmap);
        bitmapfont_draw_string_box(&ui_bitmap,
            UI_CENTERED_IN_BOX(config->x, config->width, title_box_width),
            inner_y + inner_height,
            config->title, title_box_width);
            APPEND_INCLUDING_GAP(inner_height, title_box_height);
    }
    if (config->description) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string_box(&ui_bitmap,
            UI_CENTERED_IN_BOX(config->x, config->width, desc_box_width),
            inner_y + inner_height,
            config->description, desc_box_width);
            APPEND_INCLUDING_GAP(inner_height, desc_box_height);
    }
    if (config->progress_max) {
        config->progress_y = inner_y + inner_height;
        APPEND_INCLUDING_GAP(inner_height, 1);
    }
    if (config->buttons[0]) {
        bitmapfont_set_active_font(font16_bitmap);
        config->buttons_y = inner_y + inner_height;
        ui_popup_dialog_draw_buttons(config, -1);
        APPEND_INCLUDING_GAP(inner_height, bitmapfont_get_font_height() - 1 + (BUTTON_Y_BORDER * 2));
    }

    ui_popup_dialog_draw_update(config);
}

void ui_popup_dialog_draw_update(ui_popup_dialog_config_t *config) {
    if (config->progress_max && config->progress_step) {
        uint16_t p_width = config->width - 16;
        uint16_t p = config->progress_step * (uint32_t)p_width / config->progress_max;
        bitmap_rect_fill(&ui_bitmap, config->x + 8, config->progress_y, p, 1,
            BITMAP_COLOR(MAINPAL_COLOR_BLACK, 3, BITMAP_COLOR_MODE_STORE));
    }
}

int16_t ui_popup_dialog_action(ui_popup_dialog_config_t *config, uint8_t selected_button) {
    if (config->buttons[0]) {
        uint8_t button_count = 1;
        for (int i = 1; i < UI_POPUP_DIALOG_MAX_BUTTON_COUNT; i++, button_count++) {
            if (!config->buttons[i]) break;
        }

        ui_popup_dialog_draw_buttons(config, 0);
        while (true) {
            wait_for_vblank();
            input_update();
            uint16_t keys_pressed = input_pressed;

            uint8_t prev_selected_button = selected_button;
            if (keys_pressed & KEY_LEFT) {
                if (selected_button > 0) {
                    selected_button--;
                } else {
                    selected_button = button_count - 1;
                }
            }
            if (keys_pressed & KEY_RIGHT) {
                selected_button++;
                if (selected_button >= button_count) {
                    selected_button = 0;
                }
            }
            if (prev_selected_button != selected_button) {
                ui_popup_dialog_draw_buttons(config, selected_button);
            }
            if (keys_pressed & KEY_A) {
                return selected_button;
            }
            if (keys_pressed & KEY_B) {
                ui_popup_dialog_draw_buttons(config, -1);
                return UI_POPUP_ACTION_BACK;
            }
        }
    }

    return UI_POPUP_ACTION_BACK;
}
