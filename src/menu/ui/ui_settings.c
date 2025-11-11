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
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>
#include <ws/keypad.h>
#include <ws/system.h>
#include "bitmap.h"
#include "lang_gen.h"
#include "settings.h"
#include "strings.h"
#include "ui.h"
#include "ui/ui_color_picker.h"
#include "ui/ui_dialog.h"
#include "ui_selector.h"
#include "ui_settings.h"
#include "lang.h"
#include "util/input.h"

typedef struct ui_settings_config {
    ui_selector_config_t config;
    const setting_category_t __far* category;
} ui_settings_config_t;

DEFINE_STRING_LOCAL(s_arrow, "→");

static void ui_settings_draw(struct ui_selector_config *config, uint16_t offset, uint16_t y) {
    char buf[96];

    ui_settings_config_t *sconfig = (ui_settings_config_t*) config;
    const setting_t __far* s = sconfig->category->entries[offset];

    int x_offset = 4;
    int len = x_offset + bitmapfont_draw_string(&ui_bitmap, x_offset, y, lang_keys[s->name], WS_DISPLAY_WIDTH_PIXELS - x_offset);

    if (s->type == SETTING_TYPE_CATEGORY || s->type == SETTING_TYPE_ACTION) {
        strcpy(buf, s_arrow);
    } else if (s->type == SETTING_TYPE_FLAG) {
        strcpy(buf, lang_keys[(*s->flag.value & (1 << s->flag.bit)) ? s->flag.name_true : s->flag.name_false]);
    } else if (s->type == SETTING_TYPE_CHOICE_BYTE) {
        s->choice.name(*((uint8_t*) s->choice.value), buf, sizeof(buf));
    } else if (s->type == SETTING_TYPE_COLOR) {
        snprintf(buf, sizeof(buf), s_color, *s->color.value & 0xFFF);
    }
  
    x_offset = WS_DISPLAY_WIDTH_PIXELS - x_offset - bitmapfont_get_string_width(buf, WS_DISPLAY_WIDTH_PIXELS - x_offset - len);
    bitmapfont_draw_string(&ui_bitmap, x_offset, y, buf, WS_DISPLAY_WIDTH_PIXELS - x_offset);
}

static bool ui_settings_can_select(struct ui_selector_config *config, uint16_t offset) {
    ui_settings_config_t *sconfig = (ui_settings_config_t*) config;
    const setting_t __far* s = sconfig->category->entries[offset];

    if (s->flags & SETTING_FLAG_COLOR_ONLY) {
        if (!ws_system_is_color_model()) {
            return false;
        }
    }

    return true;
}

void ui_settings(const setting_category_t __far* root_category) {
    ui_settings_config_t config = {{0}, root_category};
    bool reinit_ui = true;

    uint16_t offset_category_stack[4];
    uint8_t offset_category_stack_pos = 0;

    config.config.style = UI_SELECTOR_STYLE_16;
    config.config.draw = ui_settings_draw;
    config.config.can_select = ui_settings_can_select;
    config.config.key_mask = WS_KEY_A | WS_KEY_B | WS_KEY_START | WS_KEY_Y1;
    config.config.info_key = ws_system_get_model() == WS_MODEL_PCV2 ? LK_SETTINGS_INFO_PC2 : LK_SETTINGS_INFO_WS;

reload_menu:
    config.config.count = config.category->entry_count;

    if (reinit_ui) {
        ui_layout_bars();
    }
    ui_draw_titlebar(lang_keys[config.category->name]);
    ui_show();

    reinit_ui = false;

    while (true) {
        uint16_t keys_pressed = ui_selector(&config.config);
        const setting_t __far* s = config.category->entries[config.config.offset];

        if (keys_pressed & WS_KEY_Y1) {
            if (s->help) {
                ui_layout_bars();
                bitmap_rect_fill(&ui_bitmap, 0, 8, 28 * 8, 16 * 8, BITMAP_COLOR_4BPP(2));

                ui_draw_titlebar(lang_keys[s->name]);
                ui_draw_statusbar(NULL);
                
                bitmapfont_set_active_font(font16_bitmap);
                bitmapfont_draw_string_box(&ui_bitmap, 2, 10, lang_keys[s->help], WS_DISPLAY_WIDTH_PIXELS - 4, 0);

                input_wait_any_key();

                reinit_ui = true;
                goto reload_menu;
            }
        }

        if (keys_pressed & WS_KEY_A) {
            bool reload_required = false;
            
            if (s->type == SETTING_TYPE_CATEGORY) {
                config.category = s->category.value;
                offset_category_stack[offset_category_stack_pos++] = config.config.offset;
                config.config.offset = 0;
                reload_required = true;
            } else if (s->type == SETTING_TYPE_FLAG) {
                *s->flag.value ^= (1 << s->flag.bit);
            } else if (s->type == SETTING_TYPE_CHOICE_BYTE) {
                uint8_t value = *((uint8_t*) s->choice.value);
                for (int i = 0; i < s->choice.max; i++) {
                    value++;
                    if (value > s->choice.max)
                        value = 0;

                    if (!s->choice.allowed || s->choice.allowed(value)) {
                        break;
                    }
                }
                *((uint8_t*) s->choice.value) = value;
            } else if (s->type == SETTING_TYPE_COLOR) {
                ui_color_picker(s->color.value);
                reload_required = true;
            }

            if (s->on_change) {
                s->on_change(s);
                // Immediate language changes depend on this
                reload_required = true;
            }
            if (reload_required)
                goto reload_menu;
        }
        if (keys_pressed & WS_KEY_B) {
            if (config.category == root_category) {
                break;
            }
            if (config.category->parent) {
                config.category = config.category->parent;
                if (offset_category_stack_pos)
                    config.config.offset = offset_category_stack[--offset_category_stack_pos];
                goto reload_menu;
            }
        }
        if (keys_pressed & WS_KEY_START) {
            break;
        }
    }

    ui_dialog_error_check(settings_save(), NULL, 0);
}
