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
 
#ifndef _MCU_H_
#define _MCU_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include <nile.h>

int16_t mcu_reset(bool flash);
bool mcu_native_send_cmd(uint16_t cmd, const void *buffer, int buflen);

static inline bool mcu_native_set_eeprom_type(uint8_t type) {
	uint8_t tmp;
	if (!mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x10, type), NULL, 0))
		return false;
	if (!nile_mcu_native_recv_cmd(&tmp, 1))
		return false;
	return true;
}

bool mcu_native_set_mode(uint8_t mode);

static inline bool mcu_native_finish(void) {
	return nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
}

#endif /* _MCU_H_ */
