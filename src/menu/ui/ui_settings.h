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

#ifndef __UI_SETTINGS_H__
#define __UI_SETTINGS_H__

#include <ws.h>
#include "settings.h"
#include "ui.h"

uint16_t ui_settings_selector(const setting_t __far *setting, uint16_t prev_value);
void ui_settings(const setting_category_t __far* root_category);

#endif /* __UI_SETTINGS_H__ */
