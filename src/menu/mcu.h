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
#include <stddef.h>
#include <stdint.h>
#include <wonderful.h>
#include <nile.h>

#define SAVE_ID_FOR_EEPROM 0x1
#define SAVE_ID_FOR_SRAM   0x2
#define SAVE_ID_FOR_FLASH  0x200
#define SAVE_ID_FOR_ALL    0x203
#define SAVE_ID_NONE       ((uint32_t) 0xFFFFFFFF)

int16_t mcu_reset(bool flash);
bool mcu_native_send_cmd(uint16_t cmd, const void *buffer, int buflen);

bool mcu_native_save_id_set(uint32_t id, uint16_t target);
bool mcu_native_save_id_get(uint32_t *id, uint16_t target);

static inline bool mcu_native_eeprom_set_type(uint8_t type) {
	uint8_t tmp;
	if (!mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x10, type), NULL, 0))
		return false;
	if (!nile_mcu_native_recv_cmd(&tmp, 1))
		return false;
	return true;
}

static inline bool mcu_native_eeprom_read_data(uint8_t *buffer, uint16_t offset, uint16_t words) {
	if (!mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x12, words), &offset, 2))
		return false;
	if (!nile_mcu_native_recv_cmd(buffer, words * 2))
		return false;
	return true;
}

bool mcu_native_set_mode(uint8_t mode);

static inline bool mcu_native_finish(void) {
	return nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_NONE);
}

#endif /* _MCU_H_ */
