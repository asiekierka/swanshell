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

#include "cart/status.h"
#include "cart/mcu.h"
#include "errors.h"
#include "status.h"
#include <nile/flash.h>
#include <nile/flash_layout.h>
#include <nile/hardware.h>
#include <nile/mcu.h>
#include <nile/mcu/protocol.h>
#include <string.h>

cart_status_t cart_status;

#define VERSION_AT_LEAST(_major,_minor,_patch) \
    (version.major > (_major) || (version.major == (_major) && (version.minor > (_minor) || (version.minor == (_minor) && version.patch >= (_patch)))))

bool cart_status_fetch_version(void *version, size_t version_size) {
    bool is_safe_mode = (cart_status.present & CART_PRESENT_SAFE_MODE) != 0;
	nile_flash_wake();
    bool result = nile_flash_read(version, is_safe_mode ? NILE_FLASH_LAYOUT_MANIFEST_FACTORY_ADDR : NILE_FLASH_LAYOUT_MANIFEST_ADDR, version_size);
    nile_flash_sleep();
    return result;
}

static int16_t cart_status_init_inner(bool is_mcu_reset_ok) {
    // Check board revision
    if (inportb(IO_NILE_BOARD_REVISION) > 0x2) return ERR_UNSUPPORTED_CARTRIDGE_REVISION;

    // Load installed firmware version
    nile_flash_version_t version;
    if (!cart_status_fetch_version(&version, sizeof(version))) return ERR_FLASH_COMM_FAILED;
    if (version.partial_install) return ERR_INVALID_FIRMWARE_INSTALLATION;
    if (version.major < 1) return ERR_UNSUPPORTED_FIRMWARE_VERSION;

    cart_status.present |= CART_PRESENT_FLASH;
    if (VERSION_AT_LEAST(2,0,0)) return ERR_UNSUPPORTED_FIRMWARE_VERSION;
    else if (VERSION_AT_LEAST(1,1,0)) cart_status.version = CART_FW_VERSION_1_1_0;
    else cart_status.version = CART_FW_VERSION_1_0_0;

    if (is_mcu_reset_ok) cart_status.present |= CART_PRESENT_MCU;

    // Initialize stub MCU status if the cartridge is < 1.1.0
    if (cart_status.version == CART_FW_VERSION_1_0_0) {
        cart_status.mcu_info.caps = NILE_MCU_NATIVE_INFO_CAP_ACCEL
            | NILE_MCU_NATIVE_INFO_CAP_BATTERY
            | NILE_MCU_NATIVE_INFO_CAP_USB
            | NILE_MCU_NATIVE_INFO_CAP_RTC
            | NILE_MCU_NATIVE_INFO_CAP_EEPROM;
        cart_status.mcu_info.bat_voltage = 4095;
        cart_status.mcu_info.status = NILE_MCU_NATIVE_INFO_RTC_ENABLED
            | NILE_MCU_NATIVE_INFO_RTC_LSE
            | NILE_MCU_NATIVE_INFO_USB_DETECT
            | NILE_MCU_NATIVE_INFO_USB_CONNECT; 
    }

    return 0;
}

int16_t cart_status_init(bool is_safe_mode, bool is_mcu_reset_ok) {
    memset(&cart_status, 0, sizeof(cart_status_t));
    if (is_safe_mode) {
        cart_status.present |= CART_PRESENT_SAFE_MODE;
    }

    int16_t result;
    if ((result = cart_status_init_inner(is_mcu_reset_ok))) {
        // Cartridge status initialization failed.
        // This can mean an unsupported prototype cartridge revision (pre-rev6),
        // firmware corruption, or an MCU failure.
        // In such situations, we pretend the cartridge cannot do anything
        // special.
    }
    if (!result && !is_mcu_reset_ok) {
        return ERR_MCU_COMM_FAILED;
    }
    return result;
}

void cart_status_update(void) {
    if (cart_status.version < CART_FW_VERSION_1_1_0) return;

    mcu_native_start();
    nile_mcu_native_mcu_get_info_sync(&cart_status.mcu_info, sizeof(nile_mcu_native_info_t));
    mcu_native_finish();
}