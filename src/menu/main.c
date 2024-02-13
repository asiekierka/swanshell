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

#include <stdio.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include <wsx/planar_unpack.h>
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "util/input.h"

volatile uint16_t vbl_ticks;
bool is_pcv2; // TODO

__attribute__((interrupt))
void __far vblank_int_handler(void) {
	ws_hwint_ack(HWINT_VBLANK);
	cpu_irq_enable();
	vbl_ticks++;
	vblank_input_update();
}

FATFS fs;

/* static uint8_t disk_read_error;
static uint16_t bench_disk_read(uint16_t sectors) {
	outportw(IO_HBLANK_TIMER, 65535);
	outportb(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_ONESHOT);
	disk_read_error = disk_read(0, MK_FP(0x1000, 0x0000), 0, sectors);
	outportb(IO_TIMER_CTRL, 0);
	return inportw(IO_HBLANK_COUNTER) ^ 0xFFFF;
} */

void main(void) {
	outportb(IO_INT_NMI_CTRL, 0);

	cpu_irq_disable();
	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();

	// TODO: PCv2 detect

	ui_init();

	uint8_t result;
	result = f_mount(&fs, "", 1);
	if (result != FR_OK) {
		while(1);
	}

	if (ws_system_color_active()) {
		outportb(IO_SYSTEM_CTRL2, inportb(IO_SYSTEM_CTRL2) & ~(SYSTEM_CTRL2_SRAM_WAIT | SYSTEM_CTRL2_CART_IO_WAIT));
	}

	ui_file_selector();
/*	char text[60];
	for (int i = 1; i <= 14; i++) {
		uint16_t lines = bench_disk_read(i);
		sprintf(text, "%d sectors = %02X / %d lines", i, disk_read_error, lines);
		bitmapfont_draw_string(&ui_bitmap, 0, (i - 1) * 10, text, 222);
	} */

//	ui_init();
//	ui_file_selector();

	while(1);
}
