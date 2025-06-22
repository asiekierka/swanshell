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

#ifndef __UI_OSK_H__
#define __UI_OSK_H__

#include <ws.h>
#include "ui.h"

#define UI_OSK_MAX_ROWS 6

enum {
    UI_OSK_FR_ALPHA,
    UI_OSK_FR_OK,
    UI_OSK_FR_COUNT
};

typedef struct {
    char* buffer;
    uint16_t bufpos;
    uint16_t buflen;
    int8_t x, y;
    uint8_t tab, width, height;
    uint8_t row_width[UI_OSK_MAX_ROWS];
    uint8_t fr_xpos[UI_OSK_FR_COUNT];
} ui_osk_state_t;

void ui_osk(ui_osk_state_t *state);

#endif /* __UI_OSK_H__ */
