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

#ifndef __UI_DIALOG_H__
#define __UI_DIALOG_H__

#include <stdint.h>
#include <wonderful.h>

typedef struct ui_dialog_config {
    const char __far* title;
    const char __far* description;

    uint16_t progress_step;
    uint16_t progress_max;

    // Auto-filled (if width/height are zero)
    uint8_t x, y, width, height;

    // Auto-filled
    uint8_t progress_y;
} ui_dialog_config_t;

void ui_dialog_reset(ui_dialog_config_t *config);
void ui_dialog_clear(ui_dialog_config_t *config);
void ui_dialog_draw(ui_dialog_config_t *config);
void ui_dialog_draw_update(ui_dialog_config_t *config);

#endif /* __UI_DIALOG_H__ */
