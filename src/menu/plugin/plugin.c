/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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

#include <string.h>
#include "plugin/plugin.h"
#include "ui/ui.h"

void ui_draw_titlebar_filename(const char *path) {
    const char *filename_ptr = (const char*) strrchr(path, '/');
    if (filename_ptr == NULL) filename_ptr = path; else filename_ptr++;

    ui_draw_titlebar(filename_ptr);
}