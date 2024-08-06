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
#include <nilefs.h>

/**
 * @brief INI file parsing result.
 */
typedef enum {
    /**
     * @brief File parsing finished.
     */
    INI_NEXT_FINISHED,
    /**
     * @brief File parsing failed.
     */
    INI_NEXT_ERROR,
    /**
     * @brief Parsed a category. "key" points to the category name.
     */
    INI_NEXT_CATEGORY,
    /**
     * @brief Parsed a key/value pair.
     */
    INI_NEXT_KEY_VALUE
} ini_next_result_t;

/**
 * @brief Parse the next line of an INI file.
 *
 * ; comments, [categories] and key=value pairs are supported.
 * 
 * @param file File to parse.
 * @param buffer Line buffer.
 * @param buffer_size Line buffer size, in bytes.
 * @param key Key pointer store location.
 * @param value Value pointer store location
 * @return ini_next_result_t Parsing result.
 */
ini_next_result_t ini_next(FIL *file, char *buffer, uint16_t buffer_size, char **key, char **value);

#endif /* _INI_H_ */
