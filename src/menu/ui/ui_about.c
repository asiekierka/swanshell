/**
 * Copyright (c) 2025, 2026 Adrian Siekierka
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

#include <nile/flash.h>
#include <nile/mcu.h>
#include <nile/mcu/protocol.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <nile.h>
#include <ws.h>
#include <ws/display.h>
#include "bitmap.h"
#include "cart/mcu.h"
#include "cart/status.h"
#include "lang.h"
#include "lang_gen.h"
#include "strings.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "util/input.h"
#include "util/util.h"
#include "ui_about.h"

static const char __far s_name_version[] = "%s " VERSION;

void ui_about(void) {
    char buf[129];

    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SETTINGS_ABOUT]);
    ui_draw_statusbar(NULL);

    int y = 23;
    sprintf(buf, s_name_version, lang_keys[LK_NAME]);
    bitmapfont_set_active_font(font16_bitmap);
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, 65535)) >> 1,
        y, buf, 65535);

    y += 14;
    strcpy(buf, lang_keys[LK_NAME_COPYRIGHT]);
    bitmapfont_set_active_font(font8_bitmap);
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, 65535)) >> 1,
        y, buf, 65535);

    y += 11;
    sprintf(buf, lang_keys[LK_ABOUT_RAM_BYTES_FREE], mem_query_free());
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, 65535)) >> 1,
        y, buf, 65535);

    y += 19;
    bitmapfont_draw_string_box(&ui_bitmap, 4, y, lang_keys[LK_NAME_COPYRIGHT_INFO], WS_DISPLAY_WIDTH_PIXELS - 8, 1, 0);

    input_wait_any_key();
}

static int get_board_revision(void) {
    switch (inportb(IO_NILE_BOARD_REVISION)) {
    case 0: return 6;
    case 1: return 7;
    case 2: return 9;
    default: return -1;
    }
}


#define TEXT_X_START 4
#define TEXT_Y_START 12
#define UI_ABOUT_CARTRIDGE_SPACING 3
#define UI_ABOUT_CARTRIDGE_COMMA_SPACE { text_x += bitmapfont_draw_string(&ui_bitmap, text_y, text_y, s_comma, 65535) + UI_ABOUT_CARTRIDGE_SPACING; }
#define UI_ABOUT_CARTRIDGE_NEWLINE { text_x = TEXT_X_START; text_y += bitmapfont_get_font_height(); }

static int print_cartridge_version(char *buf, int text_x, int text_y) {
    nile_flash_manifest_t manifest;
    bool is_safe_mode = (cart_status.present & CART_PRESENT_SAFE_MODE) != 0;
    if (!cart_status_fetch_version(&manifest, sizeof(manifest))) {
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_ERROR], 65535);
        return text_x;
    }
    sprintf(buf, s_my_firmware_version,
        manifest.version.major,
        manifest.version.minor,
        manifest.version.patch,
        (int) manifest.commit_id[0],
        (int) manifest.commit_id[1],
        (int) manifest.commit_id[2],
        (int) manifest.commit_id[3]);

    text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, buf, 65535);
    if (is_safe_mode) {
        UI_ABOUT_CARTRIDGE_COMMA_SPACE;
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_FIRMWARE_VERSION_FACTORY], 65535);
    }
    if (manifest.version.partial_install) {
        UI_ABOUT_CARTRIDGE_COMMA_SPACE;
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_FIRMWARE_VERSION_PARTIAL_INSTALL], 65535);
    }

    return text_x;
}

static int print_mcu_protocol_version(char *buf, int text_x, int text_y) {
    nile_mcu_native_version_t version;
    mcu_native_start();
    int16_t result = nile_mcu_native_mcu_get_version_sync(&version, sizeof(version));
    mcu_native_finish();

    if (result < 4) {
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_ERROR], 65535);
    } else {
        sprintf(buf, s_my_mcu_protocol_version, version.major, version.minor);
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, buf, 65535);
    }

    return text_x;
}

static int print_rtc_status(char *buf, int text_x, int text_y) {
    if (cart_status_mcu_info_valid()) {
        if (cart_status.mcu_info.caps & NILE_MCU_NATIVE_INFO_CAP_RTC) {
            if (cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_RTC_ENABLED) {
                text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_RTC_STATUS_INITIALIZED], 65535);
                if (!(cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_RTC_LSE)) {
                    UI_ABOUT_CARTRIDGE_COMMA_SPACE;
                    text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_RTC_STATUS_REDUCED_ACCURACY], 65535);
                }
            } else {
                text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_RTC_STATUS_NOT_INITIALIZED], 65535);
            }
        } else {
            text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_NOT_SUPPORTED], 65535);
        }
    } else {
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_ERROR], 65535);
    }

    return text_x;
}

static int print_usb_status(char *buf, int text_x, int text_y) {
    if (cart_status_mcu_info_valid()) {
        if (cart_status.mcu_info.caps & NILE_MCU_NATIVE_INFO_CAP_USB) {
            if (cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_USB_CONNECT) {
                text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_USB_STATUS_CONNECTED], 65535);
            } else if (cart_status.mcu_info.status & NILE_MCU_NATIVE_INFO_USB_DETECT) {
                text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_USB_STATUS_DETECTED], 65535);
            } else {
                text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_USB_STATUS_NOT_CONNECTED], 65535);
            }
        } else {
            text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_NOT_SUPPORTED], 65535);
        }
    } else {
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_ERROR], 65535);
    }

    return text_x;
}


void ui_about_cartridge(void) {
    char buf[129];

    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SETTINGS_SYS_INFO_MY_CARTRIDGE]);
    ui_draw_statusbar(NULL);

    int text_x = TEXT_X_START;
    int text_y = TEXT_Y_START;
    bitmapfont_set_active_font(font8_bitmap);

    text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_BOARD_REVISION], 65535);
    text_x += UI_ABOUT_CARTRIDGE_SPACING;

    int brev = get_board_revision();
    if (brev < 0) strcpy(buf, lang_keys[LK_MY_CARTRIDGE_ERROR]); else sprintf(buf, s_my_board_revision, brev);
    text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, buf, 65535);

    UI_ABOUT_CARTRIDGE_NEWLINE;

    text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_FIRMWARE_VERSION], 65535);
    text_x += UI_ABOUT_CARTRIDGE_SPACING;

    text_x = print_cartridge_version(buf, text_x, text_y);

    UI_ABOUT_CARTRIDGE_NEWLINE;

    cart_status_update();

    text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_MCU_PROTOCOL_VERSION], 65535);
    text_x += UI_ABOUT_CARTRIDGE_SPACING;

    text_x = print_mcu_protocol_version(buf, text_x, text_y);

    UI_ABOUT_CARTRIDGE_NEWLINE;
    UI_ABOUT_CARTRIDGE_NEWLINE;

    if (cart_status.version >= CART_FW_VERSION_1_1_0) {
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_RTC_STATUS], 65535);
        text_x += UI_ABOUT_CARTRIDGE_SPACING;

        text_x = print_rtc_status(buf, text_x, text_y);

        UI_ABOUT_CARTRIDGE_NEWLINE;

        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, lang_keys[LK_MY_CARTRIDGE_USB_STATUS], 65535);
        text_x += UI_ABOUT_CARTRIDGE_SPACING;

        text_x = print_usb_status(buf, text_x, text_y);

        UI_ABOUT_CARTRIDGE_NEWLINE;
    }

    bitmapfont_set_active_font(font16_bitmap);
    text_y = 136 - bitmapfont_get_font_height();

    nile_flash_wake();
    if (nile_flash_read_uuid((uint8_t*) buf + 64)) {
        for (int i = 0; i < 8; i++) {
            sprintf(buf + (i * 2), s_hex_byte, (int) ((uint8_t) buf[64 + i]));
        }
        text_x = (WS_DISPLAY_WIDTH_PIXELS - bitmapfont_get_string_width(buf, WS_DISPLAY_WIDTH_PIXELS)) >> 1;
        text_x += bitmapfont_draw_string(&ui_bitmap, text_x, text_y, buf, 65535);
    }
    nile_flash_sleep();

    input_wait_any_key();
}
