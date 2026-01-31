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
#include <nile.h>
#include <nilefs.h>
#include "cluster_read.h"
#include "patches.h"
#include "bootstub.h"
#include "util/math.h"

#define SCREEN ((uint16_t *) 0x2800)

// memcpy_expand_8_16.s
extern void memcpy_expand_8_16(void *dst, const void *src, uint16_t count, uint16_t fill_value);

/* === Error reporting === */

static const char fatfs_error_header[] = "TF card read failed (  )";
static const uint8_t hexchars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

extern uint8_t diskio_detail_code;

__attribute__((noreturn))
static void report_fatfs_error(uint8_t result) {
	uint8_t buffer[12];

	// print FatFs error
	outportw(WS_SCR_PAL_0_PORT, 0x5207);
	outportw(WS_SCR_PAL_3_PORT, 0x7777);
	memcpy_expand_8_16(SCREEN + (2 * 32) + 2, fatfs_error_header, sizeof(fatfs_error_header) - 1, 0x0100);
	ws_screen_put_tile(SCREEN, 23, 2, hexchars[result >> 4] | 0x0100);
	ws_screen_put_tile(SCREEN, 24, 2, hexchars[result & 0xF] | 0x0100);

	const char *error_detail = NULL;
	switch (result) {
		case FR_DISK_ERR: error_detail = "Storage I/O error"; break;
		case FR_INT_ERR: case FR_INVALID_PARAMETER: error_detail = "Internal error"; break;
		case FR_NOT_READY: error_detail = "Storage not ready"; break;
		case FR_NO_FILE: case FR_NO_PATH: error_detail = "File not found"; break;
		case FR_NO_FILESYSTEM: error_detail = "FAT filesystem not found"; break;
	}
	if (error_detail != NULL) {
		memcpy_expand_8_16(SCREEN + ((16 - 2) * 32) + ((28 - strlen(error_detail)) >> 1), error_detail, strlen(error_detail), 0x0100);
	}

	// try fetching flash ID
	nile_flash_wake();
	result = nile_flash_read_uuid(buffer);
	nile_flash_sleep();
	if (result) {
		for (int i = 0; i < 8; i++) {
			ws_screen_put_tile(SCREEN, 6 + i*2, 15, hexchars[buffer[i] >> 4] | 0x0100);
			ws_screen_put_tile(SCREEN, 7 + i*2, 15, hexchars[buffer[i] & 0xF] | 0x0100);
		}
	}

	// deinitialize hardware
	outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART);
	outportb(IO_NILE_POW_CNT, NILE_POW_MCU_RESET);
	ia16_disable_irq();
	while(1) ia16_halt();
}

/* === Visual flair === */

static uint16_t bank_count;
static uint16_t bank_count_max;
static uint16_t progress_pos;

#define PROGRESS_BAR_Y 13

static void progress_init(uint16_t graphic, uint16_t max_value) {
	if (ws_system_is_color_active()) {
		for (int i = 0; i < 12; i++) {
			WS_DISPLAY_COLOR_MEM(i)[0] = 0xFFF;
			WS_DISPLAY_COLOR_MEM(i)[1] = 0x000;
			WS_DISPLAY_COLOR_MEM(i)[2] = 0x555;
			WS_DISPLAY_COLOR_MEM(i)[3] = 0xAAA;
		}
		if (graphic == 0) {
			for (int i = 0; i < 12; i++)
				WS_DISPLAY_COLOR_MEM(i)[3] = (!(i & 3)) ? 0xCC0 : 0xDD2;
		}
	} else {
		ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT_DEFAULT);
		for (int i = 0; i < 12; i++)
			outportw(WS_SCR_PAL_PORT(i), 0x2570);
	}

	outportb(WS_SCR_BASE_PORT, WS_SCR_BASE_ADDR1(SCREEN) | WS_SCR_BASE_ADDR2(SCREEN));
	// Initialize screen 1
	outportw(WS_SCR1_SCRL_X_PORT, 0);
	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, 28, 18);
	if (graphic != 0xFFFF) {
		for (int i = 0; i < 12; i++) {
			ws_screen_put_tile(SCREEN, 0x180 + graphic * 12 + i + WS_SCREEN_ATTR_PALETTE(i), (i & 3) + 12, (i >> 2) + 7);
		}

		// Initialize screen 2
		ws_screen_fill_tiles(SCREEN, ((uint8_t) '-') | 0x100, 0, 31, 28, 1);
		outportw(WS_SCR2_SCRL_X_PORT, ((31 - PROGRESS_BAR_Y) << 11));
		outportw(WS_SCR2_WIN_X1_PORT, (6 | (PROGRESS_BAR_Y << 8)) << 3);
		outportw(WS_SCR2_WIN_X2_PORT, ((6 | ((PROGRESS_BAR_Y + 1) << 8)) << 3) - 0x101);

		// Show screens
		outportw(WS_DISPLAY_CTRL_PORT, WS_DISPLAY_CTRL_SCR1_ENABLE | WS_DISPLAY_CTRL_SCR2_ENABLE | WS_DISPLAY_CTRL_SCR2_WIN_INSIDE);
	} else {
		outportw(WS_DISPLAY_CTRL_PORT, WS_DISPLAY_CTRL_SCR1_ENABLE);
	}

	bank_count = 0;
	bank_count_max = max_value;
	progress_pos = 0;
}

void progress_tick(void) {
	if (!bank_count_max) return;
	uint16_t progress_end = ((uint32_t)(++bank_count) << 7) / bank_count_max;
	if (progress_end > 128) progress_end = 128;
	outportb(WS_SCR2_WIN_X2_PORT, (6 << 3) + progress_end - 1);
}

/* === Hardware configuration === */

// main_asm.s
extern void restore_cold_boot_io_state(bool disable_color_mode);

void init_launch_io_state(void) {
	restore_cold_boot_io_state(true);

	if (bootstub_data->prog_rom_type != ROM_TYPE_UNKNOWN) {
		bool expected_post_bios_init = bootstub_data->prog_rom_type != ROM_TYPE_PCV2;
		bool actual_post_bios_init = (inportb(WS_SYSTEM_CTRL_PORT) & WS_SYSTEM_CTRL_IPL_LOCK) != 0;

		if (expected_post_bios_init && !actual_post_bios_init) {
			// Adjust some I/O port values to mimic a BIOS boot
			outportb(0x07, 0);
			outportw(0x10, 0);
			outportw(0x12, 0);
			for (int i = 0x80; i <= 0x8C; i++) outportb(i, 0);
			outportb(0x8F, 0);
			outportb(0x94, 0);
			outportb(0xA0, inportb(0xA0) | 0x1);
			outportw(0xBA, 0);
			outportb(0xBE, 0);
		}
	}
}

/* === Main boot code === */

// pad_image_in_memory.s
extern void pad_image_in_memory(uint32_t last_byte, uint16_t last_bank);

__attribute__((noreturn))
extern void cold_jump(const void __far *ptr);

extern void nilefs_ipc_sync(void);

int main(void) {
	outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
	// Copy boot registers for cold_jump()
	memcpy((void*) 0x0040, MEM_NILE_IPC->boot_regs.data, 24);
	nilefs_ipc_sync();

	// Read ROM, sector by sector
	uint8_t result;
	uint32_t size = bootstub_data->prog.size;
	uint32_t rom_size = MAX(size, ((uint32_t) bootstub_data->rom_banks) << 16);
	uint32_t real_size = rom_size < 0x10000 ? 0x10000 : math_next_power_of_two(rom_size);
	uint16_t start_offset = (real_size - size);
	uint16_t start_bank = (real_size - size) >> 16;
	uint16_t total_banks = real_size >> 16;

	outportb(WS_LCD_ICON_PORT, (bootstub_data->prog_flags & 1) ? WS_LCD_ICON_ORIENT_V : WS_LCD_ICON_ORIENT_H);
	if (bootstub_data->prog.cluster) {
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);

		if (bootstub_data->prog.cluster == BOOTSTUB_CLUSTER_AT_PSRAM) {
			if (size != real_size) {
				progress_init(0, total_banks - start_bank);
				pad_image_in_memory(size - 1, total_banks - 1);
			} else {
				progress_init(0xFFFF, total_banks - start_bank);
			}
		} else {
			progress_init(0, (total_banks - start_bank) * 2 - (start_offset >= 0x8000 ? 1 : 0));

			uint16_t bank = start_bank;
			uint16_t offset = start_offset; 

			cluster_open(bootstub_data->prog.cluster);
			while (bank < total_banks) {
				outportw(WS_CART_EXTBANK_RAM_PORT, bank);
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
		}

		if (bootstub_data->prog_patches & BOOTSTUB_PROG_PATCH_FREYA_SOFT_RESET) {
			patch_apply_freya_soft_reset(total_banks - 1);
		}

		outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
	}

	outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
	outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);

	// wait for vblank before clearing registers
	while ((inportb(WS_DISPLAY_LINE_PORT) + 255 - 150) < (144 + 255 - 150))
		;
	// disable display and segments first
	outportw(WS_DISPLAY_CTRL_PORT, 0);
	outportb(WS_LCD_ICON_PORT, 0);
	// disabling POW_CNT will depower TF card, reflect that in IPC
	if (!(bootstub_data->prog_pow_cnt & NILE_POW_TF)) {
		MEM_NILE_IPC->tf_card_status = 0;
	}
	init_launch_io_state();
	outportb(WS_SYSTEM_CTRL_PORT, (inportb(WS_SYSTEM_CTRL_PORT) & ~0xC) | (bootstub_data->prog_flags & 0xC));
	outportb(WS_SYSTEM_CTRL_COLOR_PORT, bootstub_data->prog_flags2);

	// deinitialize cartridge
	if (bootstub_data->prog_patches & BOOTSTUB_PROG_PATCH_FREYA_SOFT_RESET) {
		// FIXME: soft reset patch does not wake up flash, which crashes IPL0
		// as it expects flash to be ready; therefore, for now, bootstub has to
		// keep it awake during execution.
		nile_flash_wake();
	}
	outportw(IO_NILE_SEG_MASK, (0x7 << 9) | (total_banks - 1) | (bootstub_data->prog_sram_mask << 12));
	outportb(IO_NILE_EMU_CNT, bootstub_data->prog_emu_cnt);
	// POW_CNT disables cart registers, so must be last
	outportb(IO_NILE_POW_CNT, (inportb(IO_NILE_POW_CNT) & NILE_POW_MCU_RESET) | bootstub_data->prog_pow_cnt);
	// jump to cartridge
	outportb(WS_INT_ACK_PORT, 0xFF);
	cold_jump(bootstub_data->start_pointer);

error:
	report_fatfs_error(result);
}
