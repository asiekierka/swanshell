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
#include "bitmap.h"
#include "settings.h"
#include "strings.h"
#include "ui.h"
#include "ui_selector.h"
#include "ui_settings.h"
#include "lang.h"

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
    int len = x_offset + bitmapfont_draw_string(&ui_bitmap, x_offset, y, lang_keys[s->name], 224 - x_offset);

    if (s->type == SETTING_TYPE_CATEGORY) {
        strcpy(buf, s_arrow);
    } else if (s->type == SETTING_TYPE_FLAG) {
        strcpy(buf, lang_keys[(*s->flag.value & (1 << s->flag.bit)) ? s->flag.name_true : s->flag.name_false]);
    } else if (s->type == SETTING_TYPE_CHOICE_BYTE) {
        s->choice.name(*((uint8_t*) s->choice.value), buf, sizeof(buf));
    }
  
    x_offset = 224 - x_offset - bitmapfont_get_string_width(buf, 224 - x_offset - len);
    bitmapfont_draw_string(&ui_bitmap, x_offset, y, buf, 224 - x_offset);
}

void ui_settings(void) {
    ui_settings_config_t config = {{0}, &settings_root};
    bool reinit_ui = true;

    config.config.style = UI_SELECTOR_STYLE_16;
    config.config.draw = ui_settings_draw;
    config.config.key_mask = KEY_A | KEY_B | KEY_START;

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

        if (keys_pressed & KEY_A) {
            if (s->type == SETTING_TYPE_CATEGORY) {
                config.category = s->category.value;
                goto reload_menu;
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
            }

            if (s->on_change) {
                s->on_change(s);

                // Immediate language changes depend on this
                goto reload_menu;
            }
        }
        if (keys_pressed & KEY_B) {
            if (config.category->parent) {
                config.category = config.category->parent;
                goto reload_menu;
            }
        }
        if (keys_pressed & KEY_START) {
            return;
        }
    }
}
