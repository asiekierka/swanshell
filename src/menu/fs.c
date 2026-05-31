/**
 * Copyright (c) 2024, 2026 Adrian Siekierka
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

#include <ws.h>
#include <nilefs.h>
#include "lang.h"
#include "ui/ui_dialog.h"

FATFS fs;

void fs_init(void) {
	char blank = 0;
	int16_t result;
	result = f_mount(&fs, &blank, 1);
	if (result) while(1) ui_dialog_error_check(result, lang_keys[LK_ERROR_TITLE_FS_INIT], 0);
}

bool fs_initialized(void) {
    return fs.fs_type != 0;
}
