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

#define UI_CENTERED_IN_BOX(xofs,width,inner_width) ((xofs) + (((width) - (inner_width)) >> 1))

#define UI_POPUP_ACTION_BACK -1

extern bitmap_t ui_bitmap;

void ui_init(void);
void ui_hide(void);
void ui_show(void);
void ui_layout_clear(uint16_t pal);
void ui_layout_bars(void);
void ui_draw_centered_status(const char __far* text);
void ui_draw_titlebar(const char __far* text);
void ui_draw_statusbar(const char __far* text);
int ui_bmpview(const char *path);
int ui_vgmplay(const char *path);
int ui_wavplay(const char *path);

extern uint16_t bitmap_screen2[];

#define MAINPAL_COLOR_WHITE 2
#define MAINPAL_COLOR_BLACK 3
#define MAINPAL_COLOR_RED 4
#define MAINPAL_COLOR_ORANGE 5
#define MAINPAL_COLOR_YELLOW 6
#define MAINPAL_COLOR_LIME 7
#define MAINPAL_COLOR_GREEN 8
#define MAINPAL_COLOR_LIGHT_GREY 14
#define MAINPAL_COLOR_DARK_GREY 15

#endif /* __UI_H__ */
