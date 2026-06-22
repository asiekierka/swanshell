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

#include <nile/spi.h>
#include <stdbool.h>
#include <string.h>
#include <ws.h>
#include <ws/cart/rtc.h>
#include <wsx/bcd.h>
#include <nile.h>
#include "ui_rtc_clock.h"
#include "cart/mcu.h"
#include "cart/rtc.h"
#include "errors.h"
#include "lang.h"
#include "lang_gen.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "ui/ui_popup_dialog.h"
#include "main.h"
#include "util/input.h"
#include "util/util.h"

#define RTC_TIMER_GLYPH_WIDTH (8)
#define RTC_TIMER_GLYPH_HEIGHT (16)
#define RTC_TIMER_WIDTH (19*RTC_TIMER_GLYPH_WIDTH)
#define RTC_TIMER_TEXT_X (((((screen_width - RTC_TIMER_WIDTH) >> 1) & ~7)) + 8)
#define RTC_TIMER_TEXT_Y ((screen_height - 16) >> 1)
#define RTC_TIMER_TEXT_X_OFFSET 1
#define RTC_TIMER_TEXT_Y_OFFSET 2

static const uint8_t __far arrow_up_glyph[] = {
    0b00011000,
    0b00111100,
    0b01111110
};

static const uint8_t __far arrow_down_glyph[] = {
    0b01111110,
    0b00111100,
    0b00011000
};

static void draw_rtc_timer(ws_cart_rtc_datetime_t *dt, int selected_value, bool full_redraw) {
    char text[20];

    rtc_datetime_to_string(text, dt);

    if (full_redraw) {
        bitmap_rect_draw(&ui_bitmap, RTC_TIMER_TEXT_X - 2, RTC_TIMER_TEXT_Y - 1, RTC_TIMER_WIDTH + 3, RTC_TIMER_GLYPH_HEIGHT + 2, BITMAP_COLOR_4BPP(3), false);
        bitmap_rect_fill(&ui_bitmap, RTC_TIMER_TEXT_X - 1, RTC_TIMER_TEXT_Y, 1, RTC_TIMER_GLYPH_HEIGHT, BITMAP_COLOR_4BPP(2));
    }

    // int len = strlen(text);
    int len = 19;
    int x = RTC_TIMER_TEXT_X + RTC_TIMER_TEXT_X_OFFSET;
    int selected_x = selected_value + 2;
    if (selected_value >= 2)  selected_x++;
    if (selected_value >= 4)  selected_x++;
    if (selected_value >= 6)  selected_x++;
    if (selected_value >= 8)  selected_x++;
    if (selected_value >= 10) selected_x++;

    // adjust selection
    ui_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(0),
        RTC_TIMER_TEXT_X >> 3, RTC_TIMER_TEXT_Y >> 3,
        RTC_TIMER_WIDTH >> 3, RTC_TIMER_GLYPH_HEIGHT >> 3);
    ui_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(1),
        (RTC_TIMER_TEXT_X + (RTC_TIMER_GLYPH_WIDTH * selected_x)) >> 3, RTC_TIMER_TEXT_Y >> 3,
        RTC_TIMER_GLYPH_WIDTH >> 3, RTC_TIMER_GLYPH_HEIGHT >> 3);

    // write new characters
    for (int i = 0; i < len; i++, x += RTC_TIMER_GLYPH_WIDTH) {
        bitmap_rect_fill(&ui_bitmap, x - RTC_TIMER_TEXT_X_OFFSET, RTC_TIMER_TEXT_Y, RTC_TIMER_GLYPH_WIDTH, RTC_TIMER_GLYPH_HEIGHT, BITMAP_COLOR_4BPP(2));
        bitmapfont_draw_char(&ui_bitmap, x + (text[i] == ':' ? 1 : 0), RTC_TIMER_TEXT_Y + RTC_TIMER_TEXT_Y_OFFSET, text[i]);
    }

    // draw arrows
    bitmap_rect_fill(&ui_bitmap, RTC_TIMER_TEXT_X, RTC_TIMER_TEXT_Y - 6, RTC_TIMER_WIDTH, 3, BITMAP_COLOR_4BPP(0));
    bitmap_rect_fill(&ui_bitmap, RTC_TIMER_TEXT_X, RTC_TIMER_TEXT_Y + 19, RTC_TIMER_WIDTH, 3, BITMAP_COLOR_4BPP(0));
    bitmap_draw_glyph(&ui_bitmap, RTC_TIMER_TEXT_X + (RTC_TIMER_GLYPH_WIDTH * selected_x), RTC_TIMER_TEXT_Y - 6, 8, 3, 0, &arrow_up_glyph);
    bitmap_draw_glyph(&ui_bitmap, RTC_TIMER_TEXT_X + (RTC_TIMER_GLYPH_WIDTH * selected_x), RTC_TIMER_TEXT_Y + 19, 8, 3, 0, &arrow_down_glyph);
}

static int16_t ui_rtc_clock_inner(void) {
    ui_draw_titlebar(lang_keys[LK_SETTINGS_CART_SET_RTC_TIME]);
    ui_draw_statusbar(NULL);

    uint8_t last_rtc_data[RTC_DATETIME_SIZE];
    uint8_t current_rtc_data[RTC_DATETIME_SIZE];
    uint8_t new_rtc_data[RTC_DATETIME_SIZE];
    uint8_t rtc_status = WS_CART_RTC_STATUS_24_HOUR;

    mcu_native_start();

    if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_STATUS, NULL, 0, &rtc_status, 1) < 0) {
        return ERR_MCU_COMM_FAILED;
    }

    if (rtc_status & WS_CART_RTC_STATUS_POWER_LOST) {
        ui_popup_dialog_config_t dlg = {0};
        dlg.title = lang_keys[LK_DIALOG_INITIALIZING_RTC];

        idle_until_vblank();
        mcu_native_start();

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
        mcu_native_start();

        ui_popup_dialog_clear(&dlg);
    } else if (!(rtc_status & WS_CART_RTC_STATUS_24_HOUR)) {
        rtc_status |= WS_CART_RTC_STATUS_24_HOUR;
        if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_WRITE_STATUS, &rtc_status, 1, NULL, 0) < 0) {
            return ERR_MCU_COMM_FAILED;
        }
    }

    ui_layout_bars();

    if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_DATETIME, NULL, 0, current_rtc_data, RTC_DATETIME_SIZE) < RTC_DATETIME_SIZE) {
        return ERR_MCU_COMM_FAILED;
    }
    memcpy(last_rtc_data, current_rtc_data, RTC_DATETIME_SIZE);
    bool full_redraw = true;
    bool redraw = true;
    bool changed = false;

    bitmapfont_set_active_font(font16_bitmap);
    int sel_value = 0;

    while (true) {
        if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_READ_DATETIME, NULL, 0, new_rtc_data, RTC_DATETIME_SIZE) == RTC_DATETIME_SIZE) {
            if (memcmp(current_rtc_data, new_rtc_data, RTC_DATETIME_SIZE) != 0) {
                memcpy(current_rtc_data, new_rtc_data, RTC_DATETIME_SIZE);
                redraw = true;
            }
        }
        if (redraw) {
            draw_rtc_timer((ws_cart_rtc_datetime_t*) &current_rtc_data, sel_value, full_redraw);
            memcpy(last_rtc_data, current_rtc_data, RTC_DATETIME_SIZE);
            full_redraw = false;
            redraw = false;
        }

        idle_until_vblank();
        mcu_native_start();
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
            if (sel_value < 10) {
                sel_value += 2;
            } else {
                sel_value &= 1;
            }
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
            if (sel_value > 1) {
                sel_value -= 2;
            } else {
                sel_value = (sel_value & 1) + 10;
            }
            redraw = true;
        }
        if (input_pressed & WS_KEY_X3) {
            rtc_adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, rtc_status, sel_value, -1);
            changed = true;
        }
        if (input_pressed & WS_KEY_X1) {
            rtc_adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, rtc_status, sel_value, 1);
            changed = true;
        }
        if (input_pressed & WS_KEY_Y3) {
            rtc_adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, rtc_status, sel_value, -999);
            changed = true;
        }
        if (input_pressed & WS_KEY_Y1) {
            rtc_adjust_component((ws_cart_rtc_datetime_t*) &current_rtc_data, rtc_status, sel_value, 999);
            changed = true;
        }
        if (changed) {
            rtc_correct_day_of_week((ws_cart_rtc_datetime_t*) &current_rtc_data);
            if (nile_mcu_native_rtc_transaction_sync(WS_CART_RTC_CTRL_CMD_WRITE_DATETIME, current_rtc_data, RTC_DATETIME_SIZE, NULL, 0) < 0) {
                return ERR_MCU_COMM_FAILED;
            }
            changed = false;
            redraw = true;
        }
        if (input_pressed & (WS_KEY_A | WS_KEY_B | WS_KEY_START)) {
            break;
        }
    }

    return 0;
}

int16_t ui_rtc_clock(void) {
    // Initial RTC communications may take longer than the default timeout to be handled.
    uint16_t old_timeout = nile_spi_get_timeout();
    nile_spi_set_timeout(2000);
    int16_t result = ui_rtc_clock_inner();
    nile_spi_set_timeout(old_timeout);
    return result;
}
