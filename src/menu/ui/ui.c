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

#include <stdbool.h>
#include <stdio.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/memory.h>
#include <ws/system.h>
#include <wsx/planar_convert.h>
#include "bitmap.h"
#include "cart/status.h"
#include "strings.h"
#include "ui.h"
#include "ui/theme.h"
#include "settings.h"
#include "../../../build/menu/assets/menu/bar_icons.h"
#include "../../../build/menu/assets/menu/icons.h"
#include "../../../build/menu/assets/menu/screen_patterns.h"
#include "lang.h"
#include "config.h"

__attribute__((section(".iramx_1b88")))
uint16_t bitmap_screen2[32 * 18 - 4];

__attribute__((section(".iramx_2000")))
ws_display_tile_t bitmap_tiles[512];
#define bitmap_screen1 ((uint16_t ws_iram*) 0x3800)

__attribute__((section(".iramCx_4000"), retain))
ws_display_tile_4bpp_t bitmap_tiles_c1[512];
__attribute__((section(".iramCx_8000"), retain))
ws_display_tile_4bpp_t bitmap_tiles_c2[512];

bitmap_t ui_bitmap;
static uint8_t icons_visible;

// bit0-1: 0 - not checked, 2 - checked, not found; 1 - checked, found
// bit 2: 1 = translucent bars, 0 = solid bars
static uint8_t wallpaper_status;

#define UI_BAR_COLOR ((wallpaper_status & 4) ? 0 : 2)

void ui_draw_titlebar(const char __far* text) {
    bitmap_rect_fill(&ui_bitmap, 0, 0, screen_width, 8, BITMAP_COLOR_2BPP(UI_BAR_COLOR));
    if (text != NULL) {
        bitmapfont_set_active_font(font8_bitmap);
        bitmapfont_draw_string(&ui_bitmap, 2, 0, text, screen_width - 4);
    }
}

static void ui_draw_icon(int x, int idx) {
    const uint8_t __far* src;
    uint8_t *dest;
    if (!bitmap_rotation) {
        src = &gfx_bar_icons_rot[idx * 8];
        dest = ((uint8_t*) ui_bitmap.start) + (ui_bitmap.y_pitch * x * 8);
    } else {
        src = &gfx_bar_icons[idx * 8];
        dest = ((uint8_t*) ui_bitmap.start) + (ui_bitmap.x_pitch * x) + (ui_bitmap.y_pitch * (screen_height - 8));
    }
    if (ui_bitmap.bpp == 4) {
        dest[0x00] = src[0x0];
        dest[0x04] = src[0x1];
        dest[0x08] = src[0x2];
        dest[0x0c] = src[0x3];
        dest[0x10] = src[0x4];
        dest[0x14] = src[0x5];
        dest[0x18] = src[0x6];
        dest[0x1c] = src[0x7];
    } else {
        dest[0x00] = src[0x0];
        dest[0x02] = src[0x1];
        dest[0x04] = src[0x2];
        dest[0x06] = src[0x3];
        dest[0x08] = src[0x4];
        dest[0x0a] = src[0x5];
        dest[0x0c] = src[0x6];
        dest[0x0e] = src[0x7];
    }
}

uint16_t ui_icon_update(void) {
    if (!icons_visible) return screen_width;
    uint16_t prev_icon_pos = icons_visible;
    uint16_t icon_pos = (screen_width >> 3);
    bool mcu_present = cart_status.present & CART_PRESENT_MCU;
    bool mcu_info_ok = cart_status.present & CART_PRESENT_MCU_INFO_OK;
    bool mcu_data_valid = mcu_present && cart_status.version >= CART_FW_VERSION_1_1_0 && mcu_info_ok;

    if (!mcu_present) {
        ui_draw_icon(--icon_pos, UI_BAR_ICON_MCU_ERROR);
    }

    if (mcu_data_valid) {
        if (cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_BATTERY_OK) {
            ui_draw_icon(--icon_pos, UI_BAR_ICON_BATTERY_3);
        } else {
            ui_draw_icon(--icon_pos, UI_BAR_ICON_BATTERY_NONE);
        }
    }

    if (!(inportb(WS_SYSTEM_CTRL_PORT) & WS_SYSTEM_CTRL_IPL_LOCK))
        ui_draw_icon(--icon_pos, UI_BAR_ICON_BOOTROM_UNLOCK);

    if (mcu_data_valid) {
        if (cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_USB_CONNECT) {
            ui_draw_icon(--icon_pos, UI_BAR_ICON_USB_CONNECT);
        } else if (cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_USB_DETECT) {
            ui_draw_icon(--icon_pos, UI_BAR_ICON_USB_DETECT);
        }
    } else if (cart_status.present & CART_PRESENT_MCU_INFO_ERROR) {
        ui_draw_icon(--icon_pos, UI_BAR_ICON_MCU_ERROR);
    }

    if (prev_icon_pos < icon_pos) {
        for (int i = prev_icon_pos; i < icon_pos; i++)
            ui_draw_icon(i, UI_BAR_ICON_NONE);
    }
    icons_visible = icon_pos;

    return icon_pos * 8;
}

void ui_hide_icons(void) {
    icons_visible = 0;
}

void ui_draw_statusbar_lr(const char __far* text, const char __far* right_text) {
    icons_visible = right_text == NULL ? (screen_width >> 3) : 0;
    uint16_t icon_end = ui_icon_update();
    bitmap_rect_fill(&ui_bitmap, 0, screen_height-8, icon_end, 8, BITMAP_COLOR_2BPP(UI_BAR_COLOR));
    if (icon_end < screen_width && UI_BAR_COLOR) {
        bitmap_rect_fill(&ui_bitmap, icon_end, screen_height-8, screen_width-icon_end, 8, BITMAP_COLOR(2, 2, BITMAP_COLOR_MODE_STORE));
    }
    bitmapfont_set_active_font(font8_bitmap);
    if (text != NULL) {
        bitmapfont_draw_string(&ui_bitmap, 2, screen_height-8, text, icon_end - 4);
    }
    if (right_text != NULL) {
        bitmapfont_draw_string(&ui_bitmap, screen_width - 2 - bitmapfont_get_string_width(right_text, screen_width), screen_height-8, right_text, screen_width);
    }
}

void ui_draw_statusbar(const char __far* text) {
    ui_draw_statusbar_lr(text, NULL);
}

#define INIT_SCREEN_PATTERN(screen_loc, pal, ofs) \
    { \
        int ip = (ofs); \
        for (int ix = 0; ix < WS_DISPLAY_WIDTH_TILES; ix++) { \
            for (int iy = 0; iy < WS_DISPLAY_HEIGHT_TILES; iy++) { \
                ws_screen_put_tile(screen_loc, (pal) | (ip++), ix, iy); \
            } \
        } \
    }

void ui_init_palettes(void) {
    if (ws_system_is_color_active()) {
        if (ui_has_wallpaper()) {
            ws_gdma_copyp(WS_DISPLAY_COLOR_MEM(0) + 3, gfx_icons_palcolor + 6, gfx_icons_palcolor_size - 6);
        } else {
            // palette 0 - icon palette
            WS_DISPLAY_COLOR_MEM(0)[0] = (settings.accent_color_high & SETTING_THEME_DARK_MODE) ? 0x000 : 0xFFF;
            ws_gdma_copyp(WS_DISPLAY_COLOR_MEM(0) + 2, gfx_icons_palcolor + 4, gfx_icons_palcolor_size - 4);
            WS_DISPLAY_COLOR_MEM(0)[1] = 0x000;
        }

        // palette 1 - icon palette (selected)
        ws_gdma_copyp(WS_DISPLAY_COLOR_MEM(1) + 2, gfx_icons_palcolor + 4, gfx_icons_palcolor_size - 4);
        WS_DISPLAY_COLOR_MEM(1)[1] = 0xFFF;
        WS_DISPLAY_COLOR_MEM(1)[2] ^= 0xFFF;
        WS_DISPLAY_COLOR_MEM(1)[3] ^= 0xFFF;

        // palette 2 - titlebar palette
        WS_DISPLAY_COLOR_MEM(2)[0] = 0xFFF;
        WS_DISPLAY_COLOR_MEM(2)[1] = 0x000;
        WS_DISPLAY_COLOR_MEM(2)[2] = SETTING_THEME_ACCENT_COLOR_DEFAULT;
        WS_DISPLAY_COLOR_MEM(2)[3] = 0xFFF;
    } else {
        // TODO: Add gamma configuration option.
        //
        // STN (color correction):
        // WS_DISPLAY_SHADE_LUT(0, 5, 8, 11, 12, 13, 14, 15)
        //
        // IPS (no color correction):
        // WS_DISPLAY_SHADE_LUT(0, 2, 4, 6, 9, 11, 13, 15)
        ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT(0, 3, 5, 7, 9, 11, 13, 15));
        // palette 0 - icon palette
        outportw(WS_SCR_PAL_0_PORT, 0x7052);
        // palette 1 - icon palette (selected)
        outportw(WS_SCR_PAL_1_PORT, 0x0725);
        // palette 2 - titlebar palette
        outportw(WS_SCR_PAL_2_PORT, 0x0370);
    }
}

void ui_init(void) {
    ui_hide();
    icons_visible = 0;
    wallpaper_status = 0;

    // initialize palettes
#ifdef CONFIG_DEBUG_FORCE_MONO
    if (0) {
#else
    if (ws_system_is_color_model()) {
#endif
        ws_system_set_mode(WS_MODE_COLOR_4BPP);
        ui_bitmap = BITMAP(WS_TILE_4BPP_MEM(0), WS_DISPLAY_WIDTH_TILES, WS_DISPLAY_HEIGHT_TILES, 4);
    } else {
        ui_bitmap = BITMAP(WS_TILE_MEM(0), WS_DISPLAY_WIDTH_TILES, WS_DISPLAY_HEIGHT_TILES, 2);
    }
    ui_init_palettes();

    outportb(WS_SCR_BASE_PORT, WS_SCR_BASE_ADDR1(0x3800) | WS_SCR_BASE_ADDR2(bitmap_screen2));
    outportw(WS_SCR2_SCRL_X_PORT, ((14*8) << 8) + (4*8));
}

void ui_hide(void) {
    outportw(WS_DISPLAY_CTRL_PORT, 0);
}

bool ui_has_wallpaper(void) {
    return (wallpaper_status & 3) == 1;
}

__attribute__((always_inline))
static inline void ui_show_inner(void) {
    ui_init_palettes();
    uint8_t ctrl = inportb(WS_DISPLAY_CTRL_PORT) & ~0x3;
    outportb(WS_DISPLAY_CTRL_PORT, ctrl | WS_DISPLAY_CTRL_SCR2_ENABLE | (wallpaper_status & 1));
}

void ui_unload_wallpaper(void) {
    wallpaper_status = 0;
    ui_show_inner();
}

static inline bool ui_load_wallpaper(uint16_t feature) {
    INIT_SCREEN_PATTERN(bitmap_screen1, WS_SCREEN_ATTR_PALETTE(3), WS_SCREEN_ATTR_BANK(1));
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_get_bank(), {
        const uint8_t __far *data = theme_get_pointer(feature);
        if (data != NULL) {
            memcpy(WS_DISPLAY_COLOR_MEM(3), data, 32);
            WS_DISPLAY_COLOR_MEM(0)[0] = WS_DISPLAY_COLOR_MEM(3)[0];
            WS_DISPLAY_COLOR_MEM(0)[2] = WS_DISPLAY_COLOR_MEM(3)[0];

            memcpy(WS_TILE_4BPP_MEM(512), data + 32, 16128);
            return true;
        }
    });
    return false;
}

void ui_show(void) {
    if (!wallpaper_status && theme_initialized()) {
        wallpaper_status = 2;
        if (ws_system_get_mode() == WS_MODE_COLOR_4BPP) {
            if (ui_load_wallpaper(bitmap_rotation ? THEME_FEATURE_WALLPAPER_HORIZONTAL : THEME_FEATURE_WALLPAPER_VERTICAL)) {
                wallpaper_status = 1;
            }
        }
    }
    ui_show_inner();
}

void ui_layout_clear_pattern(void) {
#ifdef CONFIG_UI_PRECOMPUTED_SCREEN_PATTERNS
    memcpy(bitmap_screen2, ui_layout_clear_data, sizeof(ui_layout_clear_data));
#else
    INIT_SCREEN_PATTERN(bitmap_screen2, pal, 0);
#endif
}

void ui_layout_clear(uint16_t pal) {
    icons_visible = 0;
    if (pal == 0 && !ui_has_wallpaper()) {
        // bitmap_rect_fill(&ui_bitmap, 0, 0, screen_width, screen_height, BITMAP_COLOR_4BPP(2));
        if (ui_bitmap.bpp == 4) {
            ws_memset32(ui_bitmap.start, 0x0000FF00, WS_DISPLAY_WIDTH_PIXELS / 2 * WS_DISPLAY_HEIGHT_PIXELS);
        } else {
            ws_memset16(ui_bitmap.start, 0xFF00, WS_DISPLAY_WIDTH_PIXELS / 4 * WS_DISPLAY_HEIGHT_PIXELS);
        }
    } else {
        // bitmap_rect_clear(&ui_bitmap, 0, 0, screen_width, screen_height);
        memset(ui_bitmap.start, 0, (WS_DISPLAY_WIDTH_PIXELS / 8 * WS_DISPLAY_HEIGHT_PIXELS) * ui_bitmap.bpp);
    }

    ui_layout_clear_pattern();
}

void ui_layout_bars_pattern(void) {
#ifdef CONFIG_UI_PRECOMPUTED_SCREEN_PATTERNS
    memcpy(bitmap_screen2, !bitmap_rotation ? ui_layout_bars_v_data : ui_layout_bars_h_data, sizeof(ui_layout_clear_data));
#else
    INIT_SCREEN_PATTERN(bitmap_screen2, (!bitmap_rotation ? (ix == 0 || ix == 27) : (iy == 0 || iy == 17)) ? WS_SCREEN_ATTR_PALETTE(2) : 0, 0);
#endif
}

void ui_layout_bars(void) {
    if (!ui_has_wallpaper()) {
        bitmap_rect_fill(&ui_bitmap, 0, 8, screen_width, screen_height - 16, BITMAP_COLOR_4BPP(2));
    } else {
        bitmap_rect_clear(&ui_bitmap, 0, 8, screen_width, screen_height - 16);
    }
    ui_layout_bars_pattern();
}

void ui_screen_modify_tiles(void ws_iram *dest, uint16_t mask, uint16_t value, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (!bitmap_rotation) {
        ws_screen_modify_tiles(dest, mask, value, WS_DISPLAY_WIDTH_TILES - height - y, x, height, width);
    } else {
        ws_screen_modify_tiles(dest, mask, value, x, y, width, height);
    }
}
