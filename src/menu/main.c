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

#include <wonderful.h>
#include <ws.h>
#include <ws/util.h>
#include <wsx/planar_unpack.h>
#include <nilefs.h>
#include "lang.h"
#include "lang_gen.h"
#include "main.h"
#include "mcu.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/ui_dialog.h"
#include "ui/ui_file_selector.h"
#include "ui/ui_rtc_clock.h"
#include "launch/launch.h"
#include "util/input.h"
#include "shell/shell.h"

volatile uint16_t vbl_ticks;

__attribute__((assume_ss_data, interrupt))
void __far vblank_int_handler(void) {
	ws_int_ack(WS_INT_ACK_VBLANK);
	ia16_enable_irq();
	vbl_ticks++;
	vblank_input_update();
}

bool idle_until_vblank(void) {
	uint16_t vbl_ticks_last = vbl_ticks;

	shell_tick();
	while (vbl_ticks == vbl_ticks_last) {
		ia16_halt();
	}

	return false;
}

void wait_for_vblank(void) {
	uint16_t vbl_ticks_last = vbl_ticks;

	while (vbl_ticks == vbl_ticks_last) {
		ia16_halt();
	}
}

FATFS fs;

void fs_init(void) {
	char blank = 0;
	int16_t result;
	result = f_mount(&fs, &blank, 1);
	if (result) while(1) ui_dialog_error_check(result, lang_keys[LK_ERROR_TITLE_FS_INIT], 0);
}

void main(void) {
	ia16_disable_irq();
	ws_int_set_handler(WS_INT_VBLANK, vblank_int_handler);
	ws_int_enable(WS_INT_ENABLE_VBLANK);
	ia16_enable_irq();

	settings_reset();

	ui_init();
	ui_layout_clear(0);

	fs_init();
	outportw(WS_CART_EXTBANK_RAM_PORT, 0);

	ui_dialog_error_check(settings_load(), lang_keys[LK_ERROR_TITLE_SETTINGS_LOAD], 0);
	ui_dialog_error_check(mcu_reset(true), lang_keys[LK_ERROR_TITLE_MCU_INIT], 0);
	ui_dialog_error_check(launch_backup_save_data(), lang_keys[LK_ERROR_TITLE_SAVE_STORE], 0);

	shell_init();
	ui_file_selector();

	while(1);
}
