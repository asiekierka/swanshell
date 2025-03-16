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

#include <string.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "lang.h"
#include "strings.h"
#include "ui/ui.h"
#include "ui/ui_dialog.h"

DEFINE_STRING_LOCAL(s_mcu_path, "/NILESWAN/MCU.BIN");

FRESULT mcu_reset(bool flash) {
	FIL fp;
	uint8_t result;
	uint16_t br;
	uint8_t buffer[NILE_MCU_FLASH_PAGE_SIZE];
	ui_dialog_config_t dlg = {0};

	strcpy(buffer, s_mcu_path);
	result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
	if (result != FR_OK)
		return result;

	nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
	if (!nile_mcu_reset(flash))
		return 0xF0;

	if (flash) {
		uint32_t addr = NILE_MCU_FLASH_START;
		uint32_t to_read = f_size(&fp);
		uint16_t pages = (f_size(&fp)+NILE_MCU_FLASH_PAGE_SIZE-1)/NILE_MCU_FLASH_PAGE_SIZE;

		dlg.title = lang_keys[LK_DIALOG_UPDATING_MCU];
		dlg.progress_max = pages;

		ui_dialog_draw(&dlg);
		ui_show();

		if (!nile_mcu_boot_erase_memory(0, pages))
			return 0xF1;

		for (uint16_t i = 0; i < pages; i++, addr += NILE_MCU_FLASH_PAGE_SIZE) {
			uint16_t btr = to_read < NILE_MCU_FLASH_PAGE_SIZE ? to_read : NILE_MCU_FLASH_PAGE_SIZE;
			to_read -= NILE_MCU_FLASH_PAGE_SIZE;

			nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
			if ((result = f_read(&fp, buffer, btr, &br)) != FR_OK) {
				f_close(&fp);
				return result;
			}

			nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
			if (!nile_mcu_boot_write_memory(addr, buffer, btr)) {
				nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
				f_close(&fp);
				return 0xF2;
			}

			dlg.progress_step = i+1;
			ui_dialog_draw_update(&dlg);
		}

		if (!nile_mcu_boot_jump(NILE_MCU_FLASH_START))
			return 0xF3;
	}

	for (int i = 0; i < 5; i++)
		ws_busywait(50000);

	if (flash) {
		ui_hide();
		ui_dialog_clear(&dlg);
	}

	nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
	f_close(&fp);

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
