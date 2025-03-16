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
#include <ws.h>
#include "errors.h"
#include "lang.h"
#include "ui/ui.h"
#include "ui_popup_dialog.h"
#include "ui_error.h"

int16_t ui_error_handle(int16_t error, const char __far *title, uint16_t flags) {
    char error_name_buffer[48];
    ui_popup_dialog_config_t dlg = {0};

    if (!error) return 0;
    error_to_string_buffer(error, error_name_buffer, sizeof(error_name_buffer));

    if (title) {
        dlg.title = title;
        dlg.description = error_name_buffer;
    } else {
        dlg.title = error_name_buffer;
    }
    dlg.buttons[0] = LK_OK;
    ui_popup_dialog_draw(&dlg);
    ui_show();
    ui_popup_dialog_action(&dlg, 0);

    return 0;
}
