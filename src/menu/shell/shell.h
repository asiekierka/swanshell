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

#ifndef SHELL_H_
#define SHELL_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

void shell_init(void);
bool shell_tick(void);

enum {
    SHELL_RET_IDLE = 0,
    SHELL_RET_REFRESH_UI,
    SHELL_RET_LAUNCH_IN_PSRAM,
};

#endif /* SHELL_H_ */
