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
#include <wsx/planar_unpack.h>
#include <nilefs.h>
#include "mcu.h"
#include "settings.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "ui/ui_settings.h"
#include "launch/launch.h"
#include "util/input.h"

volatile uint16_t vbl_ticks;

__attribute__((assume_ss_data, interrupt))
void __far vblank_int_handler(void) {
	ws_hwint_ack(HWINT_VBLANK);
	cpu_irq_enable();
	vbl_ticks++;
	vblank_input_update();
}

void wait_for_vblank(void) {
	uint16_t vbl_ticks_last = vbl_ticks;

	while (vbl_ticks == vbl_ticks_last) {
		cpu_halt();
	}
}

FATFS fs;

static uint8_t disk_read_error;
static uint16_t bench_disk_read(uint16_t sectors) {
	outportw(IO_HBLANK_TIMER, 65535);
	outportb(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_ONESHOT);
	disk_read_error = disk_read(0, MK_FP(0x1000, 0x0000), 0, sectors);
	outportb(IO_TIMER_CTRL, 0);
	return inportw(IO_HBLANK_COUNTER) ^ 0xFFFF;
}

void main(void) {
	outportb(IO_INT_NMI_CTRL, 0);

	cpu_irq_disable();
	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();

	// TODO: PCv2 detect

	uint8_t result;
	result = f_mount(&fs, "", 1);
	if (result != FR_OK) {
		while(1);
	}

	ui_init();
	mcu_reset(true);
	settings_load();
	if ((result = launch_backup_save_data()) != FR_OK) {
		char text[20];
		sprintf(text, "Save restore error %d!", result);
		bitmapfont_draw_string(&ui_bitmap, 5, 5, text, 222);
		for(int i = 0; i < 30; i++) ws_busywait(60000);
	}
	ui_file_selector();
	
	while(1);
}
