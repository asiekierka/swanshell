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

#ifndef UI_H_
#define UI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bitmap.h"
#include "config.h"
#include "util/math.h"

#define UI_CENTERED_IN_BOX(xofs,width,inner_width) ((xofs) + (((width) - (inner_width)) >> 1))

#define UI_POPUP_ACTION_BACK -1
#define UI_POPUP_ACTION_START -2

extern bitmap_t ui_bitmap;

void ui_init(void);
void ui_hide(void);
void ui_show(void);
void ui_layout_clear(uint16_t pal);
void ui_layout_bars(void);
void ui_draw_titlebar(const char __far* text);
void ui_draw_statusbar(const char __far* text);
#ifdef CONFIG_ENABLE_WALLPAPER
bool ui_has_wallpaper(void);
void ui_unload_wallpaper(void);
uint16_t ui_icon_update(void);
#else
static inline bool ui_has_wallpaper(void) { return false; }
static inline void ui_unload_wallpaper(void) { }
#endif

static inline uint8_t ui_rgb_to_shade(uint16_t rgb) {
    return (math_color_to_greyscale(rgb) >> 1) ^ 7;
}

extern uint16_t bitmap_screen2[];

#define MAINPAL_COLOR_WHITE 2
#define MAINPAL_COLOR_BLACK 3
#define MAINPAL_COLOR_RED 4
#define MAINPAL_COLOR_ORANGE 5
#define MAINPAL_COLOR_YELLOW 6
#define MAINPAL_COLOR_LIME 7
#define MAINPAL_COLOR_GREEN 8
#define MAINPAL_COLOR_BLUE 11
#define MAINPAL_COLOR_LIGHT_GREY 14
#define MAINPAL_COLOR_DARK_GREY 15

#define UI_BAR_ICON_WIDTH_TILES 3
#define UI_BAR_ICON_WIDTH_PIXELS (UI_BAR_ICON_WIDTH_TILES << 3)

typedef enum {
    UI_BAR_ICON_NONE = 0,
    UI_BAR_ICON_USB_CONNECT,
    UI_BAR_ICON_BOOTROM_UNLOCK,
    UI_BAR_ICON_BATTERY_3,
    UI_BAR_ICON_BATTERY_2,
    UI_BAR_ICON_BATTERY_1,
    UI_BAR_ICON_BATTERY_0,
    UI_BAR_ICON_BATTERY_NONE,
    UI_BAR_ICON_MCU_ERROR,
    UI_BAR_ICON_USB_DETECT
} bar_icon_index_t;

#endif /* UI_H_ */
