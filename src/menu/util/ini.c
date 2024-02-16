/**
 * Copyright (c) 2022, 2023, 2024 Adrian Siekierka
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
#include "ini.h"
#include "fatfs/ff.h"

ini_next_result_t ini_next(FIL *file, char *buffer, uint16_t buffer_size, char **key, char **value) {
    while (f_gets(buffer, buffer_size, file) != NULL) {
        if (buffer[0] == '[') {
            // [category]
            char *end_pos = (char*) strchr(buffer + 1, ']');
            *end_pos = '\0';
            *key = buffer + 1;
            return INI_NEXT_CATEGORY;
        } else if (buffer[0] == ';' || buffer[0] == '#') {
            // ; comment
            continue;
        } else {
            char *value_pos = (char*) strchr(buffer, '=');
            if (value_pos) {
                char *key_pos = buffer;
                *(value_pos++) = '\0';
                *key = key_pos;
                *value = value_pos;
                return INI_NEXT_KEY_VALUE;
            }
        }
    }
    return f_eof(file) ? INI_NEXT_FINISHED : INI_NEXT_ERROR;
}
