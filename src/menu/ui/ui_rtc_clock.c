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

#include <stdbool.h>
#include <string.h>
#include <ws.h>
#include <wsx/bcd.h>
#include <nile.h>
#include "ui_rtc_clock.h"
#include "errors.h"
#include "lang.h"
#include "lang_gen.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "ui/ui_popup_dialog.h"
#include "main.h"
#include "util/input.h"
#include "util/util.h"

#define RTC_DATETIME_SIZE 7

#define RTC_TIMER_GLYPH_WIDTH (8)
#define RTC_TIMER_GLYPH_HEIGHT (16)
#define RTC_TIMER_WIDTH (19*RTC_TIMER_GLYPH_WIDTH)
#define RTC_TIMER_TEXT_X (((((WS_DISPLAY_WIDTH_PIXELS - RTC_TIMER_WIDTH) >> 1) & ~7)) + 8)
#define RTC_TIMER_TEXT_Y ((WS_DISPLAY_HEIGHT_PIXELS - 16) >> 1)
#define RTC_TIMER_TEXT_X_OFFSET 1
#define RTC_TIMER_TEXT_Y_OFFSET 2

static void draw_rtc_timer(ws_cart_rtc_datetime_t *dt, int selected_value, bool full_redraw) {
    char text[20];

    text[0]  = '2';
    text[1]  = '0';
    text[2]  = util_hex_chars[dt->date.year >> 4];
    text[3]  = util_hex_chars[dt->date.year & 0xF];
    text[4]  = '-';
    text[5]  = util_hex_chars[dt->date.month >> 4];
    text[6]  = util_hex_chars[dt->date.month & 0xF];
    text[7]  = '-';
    text[8]  = util_hex_chars[dt->date.day >> 4];
    text[9]  = util_hex_chars[dt->date.day & 0xF];
    text[10] = ' ';
    text[11] = util_hex_chars[dt->time.hour >> 4];
    text[12] = util_hex_chars[dt->time.hour & 0xF];
    text[13] = ':';
    text[14] = util_hex_chars[dt->time.minute >> 4];
    text[15] = util_hex_chars[dt->time.minute & 0xF];
    text[16] = ':';
    text[17] = util_hex_chars[dt->time.second >> 4];
    text[18] = util_hex_chars[dt->time.second & 0xF];
    text[19] = 0;

    if (full_redraw) {
        bitmap_rect_draw(&ui_bitmap, RTC_TIMER_TEXT_X - 1, RTC_TIMER_TEXT_Y - 1, RTC_TIMER_WIDTH + 2, RTC_TIMER_GLYPH_HEIGHT + 2, BITMAP_COLOR_4BPP(3), false);
    }

    int len = strlen(text);
    int x = RTC_TIMER_TEXT_X + RTC_TIMER_TEXT_X_OFFSET;
    int selected_x = selected_value + 2;
    if (selected_value >= 2)  selected_x++;
    if (selected_value >= 4)  selected_x++;
    if (selected_value >= 6)  selected_x++;
    if (selected_value >= 8)  selected_x++;
    if (selected_value >= 10) selected_x++;

    // adjust selection
    ws_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(0),
        RTC_TIMER_TEXT_X >> 3, RTC_TIMER_TEXT_Y >> 3,
        RTC_TIMER_WIDTH >> 3, RTC_TIMER_GLYPH_HEIGHT >> 3);
    ws_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(1),
        (RTC_TIMER_TEXT_X + (RTC_TIMER_GLYPH_WIDTH * selected_x)) >> 3, RTC_TIMER_TEXT_Y >> 3,
        RTC_TIMER_GLYPH_WIDTH >> 3, RTC_TIMER_GLYPH_HEIGHT >> 3);

    // write new characters
    for (int i = 0; i < len; i++, x += RTC_TIMER_GLYPH_WIDTH) {
        bitmap_rect_fill(&ui_bitmap, x - RTC_TIMER_TEXT_X_OFFSET, RTC_TIMER_TEXT_Y, RTC_TIMER_GLYPH_WIDTH, RTC_TIMER_GLYPH_HEIGHT, BITMAP_COLOR_4BPP(2));
        bitmapfont_draw_char(&ui_bitmap, x, RTC_TIMER_TEXT_Y + RTC_TIMER_TEXT_Y_OFFSET, text[i]);
    }
}

static const uint8_t __far months_in_year_bcd[] = {0x31, 0x29, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31};

static void adjust_component(ws_cart_rtc_datetime_t *dt, int sel_value, int delta) {
    int min = 0x00;
    int max = 0x99;
    int shift = (sel_value & 1) ? 0 : 4;
    uint8_t *ptr = 0;

    switch (sel_value >> 1) {
    case 0: ptr = &dt->date.year; break;
    case 1: ptr = &dt->date.month; min = 0x01; max = 0x12; break;
    case 2: ptr = &dt->date.day; min = 0x01; max = 0x31; break;
    case 3: ptr = &dt->time.hour; max = 0x23; break;
    case 4: ptr = &dt->time.minute; max = 0x59; break;
    case 5: ptr = &dt->time.second; max = 0x59; break;
    }
    if ((sel_value >> 1) == 2) {
        uint8_t month = wsx_bcd8_to_int(dt->date.month);
        if (month >= 1 && month <= 12)
            max = months_in_year_bcd[month - 1];
    }

    uint8_t old_value = *ptr;
    uint8_t new_value = 0xFF;
    int component;
adjust_component_retry:
    component = (old_value >> shift) & 0xF;
    component += delta;
    if (component < 0) {
        if (shift == 0) {
            old_value = (old_value & 0xF0) | 9;
            shift = 4;
            delta = -1;
            goto adjust_component_retry;
        } else {
            new_value = max;
        }
    }
    else if (component > 9) {
        if (shift == 0) {
            old_value = (old_value & 0xF0);
            shift = 4;
            delta = 1;
            goto adjust_component_retry;
        }
        component = 9;
    }

    if (new_value == 0xFF) new_value = (old_value & ~(0xF << shift)) | (component << shift);
    if (new_value < min) new_value = max;
    else if (new_value > max) new_value = min;

    *ptr = new_value;
}

int16_t ui_rtc_clock(void) {
    ui_draw_titlebar(lang_keys[LK_SETTINGS_CART_SET_RTC_TIME]);
    ui_draw_statusbar(NULL);

    uint8_t last_rtc_data[RTC_DATETIME_SIZE];
    uint8_t current_rtc_data[RTC_DATETIME_SIZE];
    uint8_t rtc_status = WS_CART_RTC_STATUS_24_HOUR;

	nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    // FIXME: Why does the first RTC transaction fail sometimes? Is it a timeout issue?
    if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_STATUS, NULL, 0, &rtc_status, 1) < 0) {
        if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_STATUS, NULL, 0, &rtc_status, 1) < 0) {
            return ERR_MCU_COMM_FAILED;
        }
    }

    if (rtc_status & WS_CART_RTC_STATUS_POWER_LOST) {
        ui_popup_dialog_config_t dlg = {0};
        dlg.title = lang_keys[LK_DIALOG_INITIALIZING_RTC];

        idle_until_vblank();
        ui_popup_dialog_draw(&dlg);

        if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_RESET, NULL, 0, NULL, 0) < 0) {
            return ERR_MCU_COMM_FAILED;
        }
        rtc_status = WS_CART_RTC_STATUS_24_HOUR;
        if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_WRITE_STATUS, &rtc_status, 1, NULL, 0) < 0) {
            return ERR_MCU_COMM_FAILED;
        }
        // wait slightly over a second
        for (int i = 0; i < 100; i++)
            idle_until_vblank();

        ui_popup_dialog_clear(&dlg);
    }

    ui_layout_bars();

    if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_DATETIME, NULL, 0, current_rtc_data, RTC_DATETIME_SIZE) < 7) {
        return ERR_MCU_COMM_FAILED;
    }
    memcpy(last_rtc_data, current_rtc_data, RTC_DATETIME_SIZE);
    bool full_redraw = true;
    bool redraw = true;
    bool changed = false;

    bitmapfont_set_active_font(font16_bitmap);
    int sel_value = 0;

    while (true) {
        nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_DATETIME, NULL, 0, current_rtc_data, RTC_DATETIME_SIZE);
        if (!redraw) redraw = memcmp(last_rtc_data, current_rtc_data, RTC_DATETIME_SIZE) != 0;
        if (redraw) {
            draw_rtc_timer((ws_cart_rtc_datetime_t*) &current_rtc_data, sel_value, full_redraw);

            memcpy(last_rtc_data, current_rtc_data, RTC_DATETIME_SIZE);
            full_redraw = false;
            redraw = false;
        }
        
        idle_until_vblank();
        input_update();
        
        if (input_pressed & WS_KEY_X2) {
            if (sel_value < 11) {
                sel_value++;
            } else {
                sel_value = 0;
            }
            redraw = true;
        }
        if (input_pressed & WS_KEY_Y2) {
            sel_value = 11;
            redraw = true;
        }
        if (input_pressed & WS_KEY_X4) {
            if (sel_value > 0) {
                sel_value--;
            } else {
                sel_value = 11;
            }
            redraw = true;
        }
        if (input_pressed & WS_KEY_Y4) {
            sel_value = 0;
            redraw = true;
        }
        if (input_pressed & WS_KEY_X3) {
            adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, sel_value, -1);
            changed = true;
        }
        if (input_pressed & WS_KEY_X1) {
            adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, sel_value, 1);
            changed = true;
        }
        if (input_pressed & WS_KEY_Y3) {
            adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, sel_value, -999);
            changed = true;
        }
        if (input_pressed & WS_KEY_Y1) {
            adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, sel_value, 999);
            changed = true;
        }
        if (changed) {
            if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_WRITE_DATETIME, current_rtc_data, RTC_DATETIME_SIZE, NULL, 0) < 0) {
                return ERR_MCU_COMM_FAILED;
            }
            changed = false;
        }
        if (input_pressed & (WS_KEY_A | WS_KEY_B | WS_KEY_START)) {
            break;
        }
    }

    return 0;
}