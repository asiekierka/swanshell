/**
 * Copyright (c) 2026 Adrian Siekierka
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

#ifndef LAUNCH_ROUTER_H_
#define LAUNCH_ROUTER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>

// Flags
#define LAUNCH_ROUTER_USE_SERIAL_OUT 0x0001

// Output codes
#define LAUNCH_ROUTER_OK_SRAM_UNCHANGED 0x7FFF

int16_t launch_router_file(char *path, const char *ext, uint16_t flags);

#endif /* LAUNCH_ROUTER_H_ */
