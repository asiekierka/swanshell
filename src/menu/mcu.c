/**
 * Copyright (c) 2025 Adrian Siekierka
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

#include <assert.h>
#include <nile/mcu.h>
#include <nilefs/ff.h>
#include <string.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "errors.h"
#include "lang.h"
#include "strings.h"
#include "ui/ui.h"
#include "ui/ui_popup_dialog.h"

DEFINE_STRING_LOCAL(s_mcu_path, "/NILESWAN/MCU.BIN");

static const uint8_t __wf_rom mcu_header_u0_v1[] = {'M', 'C', 'U', '0'};

#define NILE_MCU_FLASH_SIZE 262144
#define NILE_MCU_FLASH_FOOTER_START (NILE_MCU_FLASH_START + NILE_MCU_FLASH_SIZE - NILE_MCU_FLASH_PAGE_SIZE)
#define NILE_MCU_FLASH_FOOTER_PAGE ((NILE_MCU_FLASH_FOOTER_START - NILE_MCU_FLASH_START) / NILE_MCU_FLASH_PAGE_SIZE)
#define READ_BUFFER_SIZE 128

int16_t mcu_reset(bool flash) {
	FIL fp;
	uint8_t result;
	uint16_t br;
	uint8_t buffer[READ_BUFFER_SIZE];
	uint8_t read_buffer[READ_BUFFER_SIZE];
	ui_popup_dialog_config_t dlg = {0};

	if (flash) {
		strcpy(buffer, s_mcu_path);
		result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);

		// If no file is present, assume the user did not want to update the MCU.
		if (result == FR_NO_FILE) {
			flash = false;
			goto mcu_no_file;
		}

		// If the file is present, but a read issue was present, return it.
		if (result != FR_OK)
			return result;

		// Return information about an invalid MCU.BIN format.
		result = f_read(&fp, read_buffer, READ_BUFFER_SIZE, &br);
		if (result != FR_OK || memcmp(read_buffer, mcu_header_u0_v1, 4)) {
			f_close(&fp);
			return ERR_MCU_BIN_CORRUPT;
		}

		// TODO: It would be a good idea to also validate the file's size.
	}

mcu_no_file:
	nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
	if (!nile_mcu_reset(flash))
		return ERR_MCU_COMM_FAILED;

	if (flash) {
		// Compare MCU footer with MCU.BIN copy

		if (!nile_mcu_boot_read_memory(NILE_MCU_FLASH_FOOTER_START, buffer, READ_BUFFER_SIZE))
			return ERR_MCU_COMM_FAILED;

		if (!memcmp(buffer, read_buffer, READ_BUFFER_SIZE))
			goto mcu_compare_success;

		// Initialize dialog screen
		uint32_t addr = NILE_MCU_FLASH_START;
		uint32_t to_read = f_size(&fp) - READ_BUFFER_SIZE;
		uint16_t pages = (to_read+NILE_MCU_FLASH_PAGE_SIZE-1)/NILE_MCU_FLASH_PAGE_SIZE;
		uint16_t steps = (to_read+READ_BUFFER_SIZE-1)/READ_BUFFER_SIZE;

		dlg.title = lang_keys[LK_DIALOG_UPDATING_MCU];
		dlg.progress_max = steps + 1;

		ui_popup_dialog_draw(&dlg);
		ui_show();

		// Write MCU footer

		if (!nile_mcu_boot_erase_memory(NILE_MCU_FLASH_FOOTER_PAGE, 1))
			return ERR_MCU_COMM_FAILED;

		if (!nile_mcu_boot_write_memory(NILE_MCU_FLASH_FOOTER_START, read_buffer, READ_BUFFER_SIZE))
			return ERR_MCU_COMM_FAILED;

		dlg.progress_step++;
		ui_popup_dialog_draw_update(&dlg);

		// Write MCU firmware blob

		if (!nile_mcu_boot_erase_memory(0, pages))
		 	return ERR_MCU_COMM_FAILED;

		for (uint16_t i = 0; i < steps; i++, addr += READ_BUFFER_SIZE) {
			uint16_t btr = to_read < READ_BUFFER_SIZE ? to_read : READ_BUFFER_SIZE;
			to_read -= READ_BUFFER_SIZE;

			nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
			if ((result = f_read(&fp, buffer, btr, &br)) != FR_OK) {
				f_close(&fp);
				return result;
			}

			nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
			if (!nile_mcu_boot_write_memory(addr, buffer, btr)) {
				nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
				f_close(&fp);
				return ERR_MCU_COMM_FAILED;
			}

			dlg.progress_step++;
			ui_popup_dialog_draw_update(&dlg);
		}

mcu_compare_success:
		if (!nile_mcu_boot_jump(NILE_MCU_FLASH_START))
			return ERR_MCU_COMM_FAILED;
	}

	for (int i = 0; i < 5; i++)
		ws_busywait(50000);

	if (flash) {
		ui_hide();
		ui_popup_dialog_clear(&dlg);
	}

	nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);

	if (flash) {
		f_close(&fp);
	}

	return FR_OK;
}

bool mcu_native_send_cmd(uint16_t cmd, const void *buffer, int buflen) {
	if (!nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU))
		return false;
	if (!nile_spi_tx_sync_block(&cmd, 2))
		return false;
	if (buflen) {
		if (!nile_spi_tx_sync_block(buffer, buflen))
			return false;
	}
	return true;
}

bool mcu_native_set_mode(uint8_t mode) {
	bool result = mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x01, mode), NULL, 0);
	ws_busywait(2500);
	return result;
}
