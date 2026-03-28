/**
 * Copyright (c) 2026 Adrian Siekierka
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
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "bootstub.h"
#include "errors.h"
#include "launch.h"

int16_t launch_plugin_via_ipc(const char __far *plugin_path, const char *filename) {
    char filepath[0x400 - 0xE0 + 1];
    int filepath_pos;
    filepath[sizeof(filepath) - 1] = 0;

    int16_t result = f_getcwd(&filepath, sizeof(filepath));
    if (result != FR_OK)
        return result;
    filepath_pos = strlen(filepath);
    if (filepath_pos >= (sizeof(filepath) - 1))
        return FR_INVALID_OBJECT; // TODO: Valid error
    filepath[filepath_pos++] = '/';
    strncpy(filepath + filepath_pos, filename, sizeof(filepath) - filepath_pos);
    if (filepath[sizeof(filepath) - 1] != 0)
        return FR_INVALID_OBJECT; // TODO: Valid error

    // Configure IPC area
    ws_bank_with_ram(NILE_SEG_RAM_IPC, {
        strcpy(MK_FP(0x1000, 0x00E0), filepath);
    });

    // Load plugin as ROM
    strcpy(filepath, plugin_path);
    launch_rom_metadata_t meta;
    result = launch_get_rom_metadata(filepath, &meta);
    if (result == FR_OK) {
        result = launch_set_bootstub_file_entry(filepath, &bootstub_data->prog);
        if (result == FR_OK) {
            result = launch_rom_via_bootstub(&meta);
        }
    }
    return result;
}