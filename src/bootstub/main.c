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

#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include "cluster_read.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "bootstub.h"
#include "nileswan/nileswan.h"
#include "util/math.h"

#define SCREEN ((uint16_t *) 0x2800)

/* === Visual flair === */

static uint16_t bank_count;
static uint16_t bank_count_max;
static uint16_t progress_pos;

static void progress_init(uint16_t graphic, uint16_t max_value) {
	if (ws_system_color_active()) {
		MEM_COLOR_PALETTE(0)[0] = 0xFFF;
		MEM_COLOR_PALETTE(0)[1] = 0x000;
		MEM_COLOR_PALETTE(0)[2] = 0x555;
		MEM_COLOR_PALETTE(0)[3] = 0xAAA;
	} else {
		ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
		outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 5, 2));
	}

    outportw(IO_SCR1_SCRL_X, 0);
	outportb(IO_SCR_BASE, SCR1_BASE(SCREEN));
	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, 28, 18);
	for (int i = 0; i < 12; i++) {
		ws_screen_put_tile(SCREEN, 0x180 + graphic * 12 + i, (i & 3) + 12, (i >> 2) + 7);
	}
	outportw(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE);

	bank_count = 0;
	bank_count_max = max_value;
	progress_pos = 0;
}

static void progress_tick(void) {
	uint16_t progress_end = ((++bank_count) << 4) / bank_count_max;
	if (progress_end > 16) progress_end = 16;
	for (; progress_pos < progress_end; progress_pos++) {
		ws_screen_put_tile(SCREEN, ((uint8_t) '-') | 0x100, 6 + progress_pos, 13);
	}
}

/* === Hardware configuration === */

static inline void clear_registers(bool disable_color_mode) {
    // wait for vblank, disable display, reset some registers
	while (inportb(IO_LCD_LINE) != 144);

	bool color_active = ws_system_color_active();
	_nmemset((void*) 0x2000, 0x00, color_active ? 0xE000 : 0x2000);
    if (color_active) {
        uint8_t ctrl2 = disable_color_mode ? 0x0A : 0x8A;
        outportb(IO_SYSTEM_CTRL2, ctrl2);
	}

    outportw(IO_DISPLAY_CTRL, 0);
    outportb(IO_LCD_SEG, 0);
    outportb(IO_SPR_BASE, 0);
    outportb(IO_SPR_FIRST, 0);
    outportb(IO_SPR_COUNT, 0);
    // outportw(IO_SCR1_SCRL_X, 0); Already done in progress_init
    outportw(IO_SCR2_SCRL_X, 0);
    outportb(IO_HWINT_VECTOR, 0);
    outportb(IO_HWINT_ENABLE, 0);
    outportb(IO_INT_NMI_CTRL, 0);
    outportb(IO_KEY_SCAN, 0x40);
    outportb(IO_SCR_BASE, 0x26);

    outportb(IO_HWINT_ACK, 0xFF);
}

/* === Main boot code === */

__attribute__((noreturn))
extern void launch_ram_asm(const void __far *ptr);

int main(void) {
	// Read ROM, sector by sector
	uint8_t result;
	uint32_t size = bootstub_data->prog_size;
    uint32_t real_size = size < 0x10000 ? 0x10000 : math_next_power_of_two(size);
    uint16_t offset = (real_size - size);
    uint16_t bank = (real_size - size) >> 16;
    uint16_t total_banks = real_size >> 16;

	progress_init(0, (total_banks - bank) * 2 - (offset >= 0x8000 ? 2 : 1));
	cluster_open(bootstub_data->prog_cluster);
	outportb(IO_CART_FLASH, CART_FLASH_ENABLE);

	while (bank < total_banks) {
		outportw(IO_BANK_2003_RAM, bank);
		if (offset < 0x8000) {
			progress_tick();
			if ((result = cluster_read(MK_FP(0x1000, offset), 0x8000 - offset)) != FR_OK) {
                 goto error;
			}
			offset = 0x8000;
		}
		progress_tick();
		if ((result = cluster_read(MK_FP(0x1000, offset), -offset)) != FR_OK) {
			goto error;
		}
		offset = 0x0000;
		bank++;
	}
    
	outportb(IO_CART_FLASH, 0);
	outportw(IO_NILE_SPI_CNT, 0);
	outportw(IO_NILE_SEG_MASK, (total_banks - 1) | (0x7 << 12));
	clear_registers(true);
	outportb(IO_NILE_POW_CNT, 0);
	launch_ram_asm(MK_FP(0xFFFF, 0x0000));

error:
	while(1);
}
