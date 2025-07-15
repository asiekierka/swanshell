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

#include "../lang.h"
#include "mcu.h"
#include "../main.h"
#include "../ui/ui.h"
#include "../util/input.h"

int ui_hidctrl(void) {
    ui_layout_bars();
    ui_draw_titlebar(lang_keys[LK_SUBMENU_OPTION_CONTROLLER_MODE]);

    // TODO: draw any kind of visual indicator
    // TODO: more than 75 Hz polling

    ws_system_model_t model = ws_system_get_model();
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