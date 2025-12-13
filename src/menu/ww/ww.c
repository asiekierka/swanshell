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

#include <nilefs/ff.h>
#include <stddef.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>
#include "strings.h"

int16_t ww_ui_move_to_fbin(const char __far *filename) {
    char src_fn[FF_LFN_BUF+1];
    char dst_fn[FF_LFN_BUF+1];

    strcpy(dst_fn, s_path_fbin);
    int result = f_mkdir(dst_fn);
    if (result != FR_OK && result != FR_EXIST)
        return result;

    const char __far *basename = strrchr(filename, '/');
    if (basename == NULL) {
        basename = filename;
        strcat(dst_fn, s_path_sep);
    }

    strncpy(src_fn, filename, sizeof(src_fn) - 1);
    src_fn[sizeof(src_fn) - 1] = 0;

    strcat(dst_fn, basename);
    return f_rename(src_fn, dst_fn);
}