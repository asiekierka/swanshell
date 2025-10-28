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

#ifndef LAUNCH_ATHENA_H_
#define LAUNCH_ATHENA_H_

#include "launch.h"

/**
 * @brief Start loading AthenaOS environment.
 * 
 * @param path Path to .bin file containing AthenaOS.
 * @return int16_t Error code, if any.
 */
int16_t launch_athena_begin(const char __far *path);

/**
 * @brief Launch AthenaOS environment.
 * 
 * @return int16_t Error code, if any.
 */
int16_t launch_athena_jump(void);

/**
 * @brief Start creating a ROM file layout.
 * 
 * @return int16_t Error code, if any.
 */
int16_t launch_athena_romfile_begin(void);

/**
 * @brief Add file to ROM layout.
 * 
 * @param path File path.
 * @return int16_t Error code, if any.
 */
int16_t launch_athena_romfile_add(const char *path, bool is_main_executable);

#endif /* LAUNCH_ATHENA_H_ */
