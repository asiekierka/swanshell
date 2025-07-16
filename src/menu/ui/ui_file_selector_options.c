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

#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "lang.h"
#include "lang_gen.h"
#include "settings.h"
#include "ui_about.h"
#include "ui_dialog.h"
#include "ui_fileops.h"
#include "ui_popup_list.h"
#include "ui_settings.h"
#include "../plugin/plugin.h"
#include "../ww/ww.h"

int ui_file_selector_actions_bfb(void) {
    ui_popup_list_config_t lst = {0};
    lst.option[0] = lang_keys[LK_SUBMENU_OPTION_TEST_BFB];
    return ui_popup_list(&lst);
}

static bool ui_file_selector_tools_witch(ui_popup_list_config_t *lst, const char __far *filename) {
    while (true) {
        memset(lst, 0, sizeof(ui_popup_list_config_t));
        uint8_t i = 0;
        if (fileops_has_rom_contents(filename)) {
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_REPLACE_BIOS];
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_REPLACE_OS];
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_EXTRACT_BIOS_OS];
        }
        lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_CREATE_IMAGE];

        switch (ui_popup_list(lst)) {
        default:
            ui_popup_list_clear(lst);
            return false;
        case 0:
            if (i == 1)
                ui_dialog_error_check(ww_ui_create_image(), NULL, 0);
            else
                ui_dialog_error_check(ww_ui_replace_component(filename, false), NULL, 0);
            return true;
        case 1:
            ui_dialog_error_check(ww_ui_replace_component(filename, true), NULL, 0);
            return true;
        case 2:
            ui_dialog_error_check(ww_ui_extract_from_rom(filename), NULL, 0);
            return true;
        case 3:
            ui_dialog_error_check(ww_ui_create_image(), NULL, 0);
            return true;
        }
    }
}

static bool ui_file_selector_tools(ui_popup_list_config_t *lst, const char __far *filename) {
    while (true) {
        memset(lst, 0, sizeof(ui_popup_list_config_t));
        lst->option[0] = lang_keys[LK_SUBMENU_OPTION_WITCH];
        lst->option[1] = lang_keys[LK_SUBMENU_OPTION_CONTROLLER_MODE];

        switch (ui_popup_list(lst)) {
        default:
            ui_popup_list_clear(lst);
            return false;
        case 0:
            if (ui_file_selector_tools_witch(lst, filename))
                return true;
            break;
        case 1:
            ui_hidctrl();
            return true;
        }
    }
}

bool ui_file_selector_options(const char __far *filename) {
    ui_popup_list_config_t lst;

    while (true) {
        memset(&lst, 0, sizeof(ui_popup_list_config_t));
        lst.option[0] = lang_keys[LK_SUBMENU_OPTION_SETTINGS];
        lst.option[1] = lang_keys[LK_SUBMENU_OPTION_TOOLS];
        lst.option[2] = lang_keys[LK_SUBMENU_OPTION_ABOUT];
        switch (ui_popup_list(&lst)) {
        default:
            return false;
        case 0:
            ui_settings(&settings_root);
            return true;
        case 1:
            if (ui_file_selector_tools(&lst, filename))
                return true;
            break;
        case 2:
            ui_about();
            return true;
        }
    }
}
