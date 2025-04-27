/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws.h>
#include <ws/cartridge.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include <wsx/zx0.h>
#include <nile.h>
#include <nilefs.h>
#include "errors.h"
#include "launch.h"
#include "bootstub.h"
#include "mcu.h"
#include "strings.h"
#include "ui/ui.h"
#include "../../build/menu/build/bootstub_bin.h"
#include "util/file.h"

extern FATFS fs;

#define MIN_SUPPORTED_ADDRESS 0x4000

int16_t launch_bfb(const char *path) {
    FIL fp;
    uint32_t br;
    uint16_t header[2];
    
    if (!ws_system_color_active()) {
        return FR_INT_ERR;
    }

    int16_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
	if (result != FR_OK) return result;

    // Read header
    result = f_read(&fp, &header, 4, &br);
	if (result != FR_OK) return result;

    if (header[0] != 0x4662 || header[1] < MIN_SUPPORTED_ADDRESS) {
        f_close(&fp);
        return ERR_FILE_FORMAT_INVALID;
    }

    uint16_t segment = 0;
    uint16_t offset = header[1];
    if (offset == 0xFFFF) {
        segment = (MIN_SUPPORTED_ADDRESS >> 4);
        offset = 0x0000;
    }
    uint16_t max_size = 0xFE00 - (segment * 16 + offset);
    void __far* ptr = MK_FP(segment, offset);

    // Disable IRQs - avoid other code interfering/overwriting memory
    cpu_irq_disable();
    outportw(IO_DISPLAY_CTRL, 0);

    result = f_read(&fp, ptr, max_size, &br);
	if (result != FR_OK) {
        ui_init();
        return result;
    }

    f_close(&fp);

    outportb(0x60, 0x80);
    asm volatile("push %0\npush %1\nlret" :: "g" (segment), "g" (offset));
    return 0;
}
