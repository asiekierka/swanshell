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

#include <wonderful.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/system.h>

#include "../lang.h"
#include "lang_gen.h"
#include "mcu.h"
#include "../main.h"
#include "../ui/ui.h"
#include "../util/input.h"
#include "ui/bitmap.h"

int ui_hidctrl(void) {
    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SUBMENU_OPTION_CONTROLLER_MODE]);
    ui_draw_statusbar(NULL);

    bitmapfont_set_active_font(font16_bitmap);

    ws_system_model_t model = ws_system_get_model();

    uint16_t s1 = LK_CONTROLLER_MODE_ACTIVE;
    uint16_t s2 = model == WS_MODEL_PCV2 ? LK_CONTROLLER_MODE_PRESS_VIEW_TO_EXIT : LK_CONTROLLER_MODE_PRESS_SOUND_TO_EXIT;
    uint16_t width1 = bitmapfont_get_string_width(lang_keys[s1], 65535);
    uint16_t width2 = bitmapfont_get_string_width(lang_keys[s2], 65535);
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - width1) >> 1,
        (WS_DISPLAY_HEIGHT_PIXELS >> 1) - 20,
        lang_keys[s1], 65535);
    bitmapfont_draw_string(&ui_bitmap,
        (WS_DISPLAY_WIDTH_PIXELS - width2) >> 1,
        (WS_DISPLAY_HEIGHT_PIXELS >> 1) + 4,
        lang_keys[s2], 65535);

    // TODO: more than 75 Hz polling

    while (true) {
        wait_for_vblank();
        input_update();

        // test for exit key: 1 << 12 on PCv2, SOUND button on WS/WSC
        if (model == WS_MODEL_PCV2) {
            if (input_held & KEY_PCV2_VIEW)
                break;
        } else {
            if (inportb(WS_LCD_ICON_LATCH_PORT) & (WS_LCD_ICON_LATCH_VOLUME | WS_LCD_ICON_LATCH_VOLUME_A | WS_LCD_ICON_LATCH_VOLUME_B))
                break;
        }

        // send HID update to MCU
        uint16_t hid_data = (input_held >> 1) & 0x7FF;
        mcu_native_hid_update(hid_data);
    }

    // disable buttons
    mcu_native_hid_update(0);

    return 0;
}