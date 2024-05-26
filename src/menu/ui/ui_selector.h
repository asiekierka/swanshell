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

#ifndef __UI_SELECTOR_H__
#define __UI_SELECTOR_H__

#include "ui.h"

#define UI_SELECTOR_STYLE_16 0
#define UI_SELECTOR_STYLE_8  1

typedef struct ui_selector_config {
    uint16_t offset, count;
    uint16_t key_mask;
    uint8_t style;

    void (*draw)(struct ui_selector_config *config, uint16_t idx, uint16_t y);
} ui_selector_config_t;

uint16_t ui_selector(ui_selector_config_t *config);

#endif /* __UI_SELECTOR_H__ */
