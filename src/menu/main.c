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
#include <ws/util.h>
#include <wsx/planar_unpack.h>
#include <nilefs.h>
#include "lang.h"
#include "lang_gen.h"
#include "mcu.h"
#include "settings.h"
#include "ui/bitmap.h"
#include "ui/ui.h"
#include "ui/ui_error.h"
#include "ui/ui_file_selector.h"
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

void fs_init(void) {
	char blank = 0;
	int16_t result;
	result = f_mount(&fs, &blank, 1);
	if (result) while(1) ui_error_handle(result, lang_keys[LK_ERROR_TITLE_FS_INIT], 0);
}

// Reserve 0x2000 bytes of space for BIOS window
__attribute__((section(".rom0_ffff_e000.bios_pad")))
volatile uint8_t bios_pad[0x1FF0] = {0x00};

void main(void) {
	// FIXME: bios_pad[0] is used here solely to create a strong memory reference
	// to dodge elf2rom's limited section garbage collector
	outportb(IO_INT_NMI_CTRL, bios_pad[0] & 0x00);

	cpu_irq_disable();
	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();

	settings_reset();

	ui_init();
	ui_layout_clear(0);

	fs_init();

	ui_error_handle(settings_load(), lang_keys[LK_ERROR_TITLE_SETTINGS_LOAD], 0);
	ui_error_handle(mcu_reset(true), lang_keys[LK_ERROR_TITLE_MCU_INIT], 0);
	ui_error_handle(launch_backup_save_data(), lang_keys[LK_ERROR_TITLE_SAVE_STORE], 0);
	ui_file_selector();

	while(1);
}
