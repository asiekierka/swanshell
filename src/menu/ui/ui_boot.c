/**
 * Copyright (c) 2024 Adrian Siekierka
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include "fatfs/ff.h"
#include "bitmap.h"
#include "ui.h"
#include "../util/input.h"
#include "../util/util.h"

__attribute__((section(".iramx_0040")))
uint16_t ipl0_initial_regs[16];

static inline uint32_t round2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static void clear_registers(bool disable_color_mode) {
    // wait for vblank, disable display, reset some registers
    wait_for_vblank();
    cpu_irq_disable();

    if (ws_system_color_active()) {
        _nmemset((void*) 0x4000, 0x00, 0xC000);
        uint8_t ctrl2 = disable_color_mode ? 0x0A : 0x8A;
        outportb(IO_SYSTEM_CTRL2, ctrl2);
    }

    outportw(IO_DISPLAY_CTRL, 0);
    outportb(IO_LCD_SEG, 0);
    outportb(IO_SPR_BASE, 0);
    outportb(IO_SPR_FIRST, 0);
    outportb(IO_SPR_COUNT, 0);
    outportb(IO_SCR1_SCRL_X, 0);
    outportb(IO_SCR1_SCRL_Y, 0);
    outportb(IO_SCR2_SCRL_X, 0);
    outportb(IO_SCR2_SCRL_Y, 0);
    outportb(IO_HWINT_VECTOR, 0);
    outportb(IO_HWINT_ENABLE, 0);
    outportb(IO_INT_NMI_CTRL, 0);
    outportb(IO_KEY_SCAN, 0x40);
    outportb(IO_SCR_BASE, 0x26);

    outportb(IO_HWINT_ACK, 0xFF);
}

extern void launch_ram_asm(const void __far *ptr, uint16_t bank_mask);

void ui_boot(const char *path) {
    FIL fp;
    
	uint8_t result = f_open(&fp, path, FA_READ);
	if (result != FR_OK) {
        // TODO
        return;
	}

    outportw(IO_DISPLAY_CTRL, 0);
	outportb(IO_CART_FLASH, CART_FLASH_ENABLE);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return;
    }
    if (size < 65536L) {
        size = 65536L;
    }
    uint32_t real_size = round2(size);
    uint16_t offset = (real_size - size);
    uint16_t bank = (real_size - size) >> 16;
    uint16_t total_banks = real_size >> 16;

	while (bank < total_banks) {
		outportw(IO_BANK_2003_RAM, bank);
		if (offset < 0x8000) {
			if ((result = f_read(&fp, MK_FP(0x1000, offset), 0x8000 - offset, NULL)) != FR_OK) {
                ui_init();
				return;
			}
			offset = 0x8000;
		}
		if ((result = f_read(&fp, MK_FP(0x1000, offset), -offset, NULL)) != FR_OK) {
            ui_init();
			return;
		}
		offset = 0x0000;
		bank++;
	}
    
    clear_registers(true);
    launch_ram_asm(MK_FP(0xFFFF, 0x0000), (total_banks - 1) | (0x7 << 12));
}
