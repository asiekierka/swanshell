/**
 * Copyright (c) 2026 Adrian Siekierka
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
 
#ifndef CART_STATUS_H_
#define CART_STATUS_H_

#include <nile.h>
#include <stdint.h>

#define CART_PRESENT_MCU 0x01
#define CART_PRESENT_FLASH 0x02
#define CART_PRESENT_SAFE_MODE 0x80
#define CART_FW_VERSION_NONE 0x00
#define CART_FW_VERSION_1_0_0 0x01
#define CART_FW_VERSION_1_1_0 0x02

typedef struct {
    uint8_t present;
    uint8_t version;
    nile_mcu_native_info_t mcu_info;
} cart_status_t;

extern cart_status_t cart_status;

bool cart_status_fetch_version(void *version, size_t version_size);
int16_t cart_status_init(bool is_safe_mode, bool is_mcu_reset_ok);
void cart_status_update(void);

static inline bool cart_status_mcu_info_valid(void) {
    return (cart_status.present & CART_PRESENT_MCU) && (cart_status.version >= CART_FW_VERSION_1_1_0);
}

#endif /* CART_MCU_H_ */
