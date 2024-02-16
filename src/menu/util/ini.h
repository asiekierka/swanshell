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

#ifndef _INI_H_
#define _INI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>
#include "fatfs/ff.h"

typedef enum {
    INI_NEXT_FINISHED,
    INI_NEXT_ERROR,
    INI_NEXT_CATEGORY,
    INI_NEXT_KEY_VALUE
} ini_next_result_t;

ini_next_result_t ini_next(FIL *file, char *buffer, uint16_t buffer_size, char **key, char **value);

#endif /* _INI_H_ */
