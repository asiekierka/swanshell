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

#ifndef __UI_POPUP_DIALOG_H__
#define __UI_POPUP_DIALOG_H__

#include <stdint.h>
#include <wonderful.h>

#define UI_POPUP_DIALOG_MAX_BUTTON_COUNT 3
typedef struct ui_popup_dialog_config {
    const char __far* title;
    const char __far* description;

    uint16_t progress_step;
    uint16_t progress_max;
    uint16_t buttons[UI_POPUP_DIALOG_MAX_BUTTON_COUNT];

    // Auto-filled (if width/height are zero)
    uint8_t x, y, width, height;

    // Auto-filled
    uint8_t progress_y;
    uint8_t buttons_y;
} ui_popup_dialog_config_t;

void ui_popup_dialog_reset(ui_popup_dialog_config_t *config);
void ui_popup_dialog_clear(ui_popup_dialog_config_t *config);
void ui_popup_dialog_clear_progress(ui_popup_dialog_config_t *config);
void ui_popup_dialog_draw(ui_popup_dialog_config_t *config);
void ui_popup_dialog_draw_update(ui_popup_dialog_config_t *config);
int16_t ui_popup_dialog_action(ui_popup_dialog_config_t *config, uint8_t selected_button);

#endif /* __UI_POPUP_DIALOG_H__ */
