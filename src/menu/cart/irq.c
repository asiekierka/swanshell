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

#include "irq.h"
#include "cart/mcu.h"
#include "cart/status.h"
#include <nile.h>
#include <nilefs.h>

void cart_irq_update(void) {
    if (cart_status.version < CART_FW_VERSION_1_1_0)
        return;
    mcu_native_start();
    uint16_t irq = nile_mcu_native_mcu_reg_read_sync(NILE_MCU_NATIVE_REG_IRQ_STATUS_AUTOACK);
    mcu_native_finish();

    if (irq & NILE_MCU_NATIVE_IRQ_TF_INSERT) {
        nilefs_eject();
        nile_soft_reset();
    }
}
