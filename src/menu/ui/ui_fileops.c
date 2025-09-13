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
#include <stdbool.h>
#include <string.h>
#include <ws.h>
#include "errors.h"
#include "lang.h"
#include "lang_gen.h"
#include "ui/ui.h"
#include "strings.h"
#include "util/file.h"
#include "ui/ui_popup_dialog.h"

bool fileops_is_rom(const char __far *filename) {
    const char __far *filename_ext = strrchr(filename, '.');
    return filename_ext != NULL && (!strcasecmp(filename_ext, s_file_ext_ws) || !strcasecmp(filename_ext, s_file_ext_wsc) || !strcasecmp(filename_ext, s_file_ext_pc2));
}

bool fileops_has_rom_contents(const char __far *filename) {
    const char __far *filename_ext = strrchr(filename, '.');
    return filename_ext != NULL && (!strcasecmp(filename_ext, s_file_ext_ws) || !strcasecmp(filename_ext, s_file_ext_wsc) || !strcasecmp(filename_ext, s_file_ext_pc2) || !strcasecmp(filename_ext, s_file_ext_flash));
}

static int16_t __fileops_delete_path(char *path, bool hide_other_values) {
    int16_t result = f_unlink(path);
    if (result != FR_OK) {
        if (hide_other_values && (result == FR_NO_FILE || result == FR_NO_PATH)) return FR_OK;
        return result;
    }
    return FR_OK;
}

int16_t fileops_delete_file_and_savedata(const char __far *filename, bool and_savedata) {
    char path[FF_LFN_BUF+1];
    int16_t result;

    strcpy(path, filename);
    result = __fileops_delete_path(path,  false);
    if (result != FR_OK) {
        if (and_savedata && (result == FR_NO_FILE || result == FR_NO_PATH)) return FR_OK;
        return result;
    }

    if (and_savedata && fileops_is_rom(path)) {
        char *path_ext = (char*) strrchr(path, '.');

        // Delete save data
        strcpy(path_ext, s_file_ext_sram);
        if ((result = __fileops_delete_path(path, true)) != FR_OK) return result;
        strcpy(path_ext, s_file_ext_eeprom);
        if ((result = __fileops_delete_path(path, true)) != FR_OK) return result;
        strcpy(path_ext, s_file_ext_flash);
        if ((result = __fileops_delete_path(path, true)) != FR_OK) return result;
        strcpy(path_ext, s_file_ext_rtc);
        if ((result = __fileops_delete_path(path, true)) != FR_OK) return result;
    }

    return FR_OK;
}

int16_t ui_fileops_check_file_overwrite(const char __far *filename) {
    if (!f_exists_far(filename)) return FR_OK;

    ui_popup_dialog_config_t dlg = {0};

    dlg.title = lang_keys[LK_DIALOG_FILE_ALREADY_EXISTS];
    dlg.description = lang_keys[LK_DIALOG_OVERWRITE_CONFIRM];
    dlg.buttons[0] = LK_YES;
    dlg.buttons[1] = LK_NO;

    ui_popup_dialog_draw(&dlg);
    ui_show();
    
    if (ui_popup_dialog_action(&dlg, 1) == 0) {
        return fileops_delete_file_and_savedata(filename, true);
    } else {
        return FR_EXIST;
    }
}

int16_t ui_fileops_check_file_delete_by_user(const char __far *filename) {
    ui_popup_dialog_config_t dlg = {0};

    dlg.title = lang_keys[LK_SUBMENU_OPTION_FILE_DELETE];
    dlg.description = lang_keys[LK_DIALOG_FILE_DELETE_CONFIRM];
    dlg.buttons[0] = LK_YES;
    dlg.buttons[1] = LK_NO;

    ui_popup_dialog_draw(&dlg);
    ui_show();
    
    if (ui_popup_dialog_action(&dlg, 1) == 0) {
        return fileops_delete_file_and_savedata(filename, false);
    } else {
        return FR_OK;
    }
}