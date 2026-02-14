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

#include <nilefs/ff.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "bootstub.h"
#include "lang.h"
#include "lang_gen.h"
#include "launch/launch.h"
#include "settings.h"
#include "strings.h"
#include "ui/ui.h"
#include "ui_about.h"
#include "ui_dialog.h"
#include "ui_fileops.h"
#include "ui_popup_list.h"
#include "ui_settings.h"
#include "../plugin/plugin.h"
#include "../ww/ww.h"
#include "../xmodem.h"

enum tristate {
    TRISTATE_FALSE,
    TRISTATE_TRUE,
    TRISTATE_NONE
};

int ui_file_selector_actions_bfb(void) {
    ui_popup_list_config_t lst = {0};
    lst.option[0] = lang_keys[LK_SUBMENU_OPTION_TEST_BFB];
    return ui_popup_list(&lst);
}

static enum tristate ui_file_selector_tools_witch(ui_popup_list_config_t *lst, const char __far *filename) {
    while (true) {
        memset(lst, 0, sizeof(ui_popup_list_config_t));
        uint8_t i = 0;
        uint8_t options[4];
        if (fileops_has_rom_contents(filename)) {
            options[i] = 0;
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_REPLACE_BIOS];
            options[i] = 1;
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_REPLACE_OS];
            options[i] = 2;
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_EXTRACT_BIOS_OS];
        } else if (fileops_has_extension(filename, s_file_ext_il)) {
            options[i] = 3;
            lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_ADD_GLOBAL_LIBRARY];
        }
        options[i] = 4;
        lst->option[i++] = lang_keys[LK_SUBMENU_OPTION_WITCH_CREATE_IMAGE];

        int result = ui_popup_list(lst);
        if (result >= 0) {
            result = options[result];
        }
        switch (result) {
        case UI_POPUP_ACTION_START:
            return TRISTATE_FALSE;
        default:
            ui_popup_list_clear(lst);
            return TRISTATE_NONE;
        case 0:
            ui_dialog_error_check(ww_ui_replace_component(filename, false), NULL, 0);
            return TRISTATE_TRUE;
        case 1:
            ui_dialog_error_check(ww_ui_replace_component(filename, true), NULL, 0);
            return TRISTATE_TRUE;
        case 2:
            ui_dialog_error_check(ww_ui_extract_from_rom(filename), NULL, 0);
            return TRISTATE_TRUE;
        case 3:
            ui_dialog_error_check(ww_ui_move_to_fbin(filename), NULL, 0);
            return TRISTATE_TRUE;
        case 4:
            ui_dialog_error_check(ww_ui_create_image(), NULL, 0);
            return TRISTATE_TRUE;
        }
    }
}

static enum tristate ui_file_selector_tools(ui_popup_list_config_t *lst, const char __far *filename) {
    while (true) {
        memset(lst, 0, sizeof(ui_popup_list_config_t));
        lst->option[0] = lang_keys[LK_SUBMENU_OPTION_TOOLS_LAUNCH_VIA_XMODEM];
        lst->option[1] = lang_keys[LK_SUBMENU_OPTION_WITCH];
        lst->option[2] = lang_keys[LK_SUBMENU_OPTION_CONTROLLER_MODE];

        switch (ui_popup_list(lst)) {
        case UI_POPUP_ACTION_START:
            return TRISTATE_FALSE;
        default:
            ui_popup_list_clear(lst);
            return TRISTATE_NONE;
        case 0: {
            uint32_t size = 0;
            int16_t result = xmodem_recv_start(&size);
            if (result == FR_OK) {
                result = launch_in_psram(size);
            }

            ui_dialog_error_check(result, lang_keys[LK_SUBMENU_OPTION_TOOLS_LAUNCH_VIA_XMODEM], 0);
            return TRISTATE_TRUE;
        }
        case 1: {
            enum tristate result = ui_file_selector_tools_witch(lst, filename);
            if (result != TRISTATE_NONE)
                return result;
            break;
        }
        case 2:
            ui_hidctrl();
            return TRISTATE_TRUE;
        }
    }
}

static enum tristate ui_file_selector_file(ui_popup_list_config_t *lst, const char __far *filename) {
    while (true) {
        memset(lst, 0, sizeof(ui_popup_list_config_t));
        lst->option[0] = lang_keys[LK_SUBMENU_OPTION_FILE_DELETE];

        switch (ui_popup_list(lst)) {
        case UI_POPUP_ACTION_START:
            return TRISTATE_FALSE;
        default:
            ui_popup_list_clear(lst);
            return TRISTATE_NONE;
        case 0: {
            int16_t result = ui_fileops_check_file_delete_by_user(filename);
            ui_dialog_error_check(result, lang_keys[LK_SUBMENU_OPTION_FILE_DELETE], 0);
            return TRISTATE_TRUE;
        }
        }
    }
}

bool ui_file_selector_options(const char __far *filename, uint8_t attrib) {
    ui_popup_list_config_t lst;

    while (true) {
        memset(&lst, 0, sizeof(ui_popup_list_config_t));
        int i = 0;
        if (!(attrib & AM_DIR)) {
            lst.option[i++] = lang_keys[LK_SUBMENU_OPTION_FILE];
        }
        lst.option[i++] = lang_keys[LK_SUBMENU_OPTION_TOOLS];
        lst.option[i++] = lang_keys[LK_SUBMENU_OPTION_SETTINGS];
        
        int option = ui_popup_list(&lst);
        if (i == 2 && option >= 0) option++;

        enum tristate result = TRISTATE_NONE;
        switch (option) {
        default:
            result = TRISTATE_FALSE;
            break;
        case 0:
            result = ui_file_selector_file(&lst, filename);
            break;
        case 1:
            result = ui_file_selector_tools(&lst, filename);
            break;
        case 2:
            ui_settings(&settings_root);
            result = TRISTATE_TRUE;
            break;
        }

        if (result != TRISTATE_NONE)
            return result != TRISTATE_FALSE;
    }
}
