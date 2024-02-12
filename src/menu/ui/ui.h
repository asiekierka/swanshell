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

#ifndef __UI_H__
#define __UI_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bitmap.h"

extern bitmap_t ui_bitmap;

void ui_init(void);
void ui_file_selector(void);
void ui_bmpview(const char *path);
void ui_boot(const char *path);
void ui_vgmplay(const char *path);
void ui_wavplay(const char *path);

extern uint16_t bitmap_screen2[];

#endif /* __UI_H__ */
