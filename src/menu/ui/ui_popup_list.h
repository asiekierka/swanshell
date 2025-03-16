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

#ifndef __UI_POPUP_LIST_H__
#define __UI_POPUP_LIST_H__

#include <stdint.h>
#include <wonderful.h>

#define UI_POPUP_LIST_MAX_OPTION_COUNT 8
typedef struct ui_popup_list_config {
    const char __far* option[UI_POPUP_LIST_MAX_OPTION_COUNT];
} ui_popup_list_config_t;

int16_t ui_popup_list(ui_popup_list_config_t *config);

#endif /* __UI_POPUP_LIST_H__ */
 