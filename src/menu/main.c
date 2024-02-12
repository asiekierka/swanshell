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
 
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <wsx/planar_unpack.h>
#include "ui/ui.h"
#include "fatfs/ff.h"
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

static FATFS fs;

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

	ui_file_selector();

	while(1);
}
