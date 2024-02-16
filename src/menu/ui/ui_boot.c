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
#include <wsx/lzsa.h>
#include "bootstub.h"
#include "../../build/menu/build/bootstub_bin.h"
#include "../../build/menu/assets/menu/bootstub_tiles.h"
#include "fatfs/ff.h"
#include "ui.h"
#include "../util/input.h"
#include "../main.h"

__attribute__((section(".iramx_0040")))
uint16_t ipl0_initial_regs[16];

/*
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
    launch_ram_asm(MK_FP(0xFFFF, 0x0000), );
} */

extern FATFS fs;

void ui_boot(const char *path) {
    FILINFO fp;
    
	uint8_t result = f_stat(path, &fp);
	if (result != FR_OK) {
        // TODO
        return;
	}

    outportw(IO_DISPLAY_CTRL, 0);

    // Disable IRQs - avoid other code interfering/overwriting memory
    cpu_irq_disable();

    // Initialize bootstub graphics
    wsx_lzsa2_decompress((void*) 0x3200, gfx_bootstub_tiles);

    // Populate bootstub data
    bootstub_data->data_base = fs.database;
    bootstub_data->cluster_table_base = fs.fatbase;
    bootstub_data->cluster_size = fs.csize;
    bootstub_data->fat_entry_count = fs.n_fatent;
    bootstub_data->fs_type = fs.fs_type;
    
    bootstub_data->prog_cluster = fp.fclust;
    bootstub_data->prog_size = fp.fsize;

    // Jump to bootstub
    memcpy((void*) 0x00c0, bootstub, bootstub_size);
    asm volatile("ljmp $0x0000,$0x00c0\n");
}
