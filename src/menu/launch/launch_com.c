/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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

#include <nilefs/ff.h>
#include <stdbool.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "errors.h"

extern FATFS fs;

typedef struct __attribute__((packed)) {
    uint16_t int20;
    uint16_t memtop;
    uint8_t reserved_04[6];
    void __far* terminate_addr; // INT 22
    void __far* ctrl_break_addr; // INT 23
    void __far* critical_error_addr; // INT 24
    uint8_t reserved_16[0x80 - 0x16];
    uint8_t argc;
    char argv[127];
} dos_psp_t;

_Static_assert(sizeof(dos_psp_t) == 256, "Invalid DOS PSP size!");

int16_t launch_com(const char *path) {
    FIL fp;
    uint32_t br;
    
    int16_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
	if (result != FR_OK) return result;
    if (f_size(&fp) > 0xFF00) return ERR_FILE_TOO_LARGE;

    // Read code
    outportw(WS_CART_EXTBANK_RAM_PORT, 0);
    result = f_read(&fp, MK_FP(0x1000, 0x0100), f_size(&fp), &br);
	if (result != FR_OK) return result;

    // Disable IRQs - avoid other code interfering/overwriting memory
    ia16_disable_irq();
    outportw(WS_DISPLAY_CTRL_PORT, 0);

    // Initialize PSP segment
    dos_psp_t __far *psp = (dos_psp_t __far*) MK_FP(0x1000, 0x0000);
    psp->int20 = 0x20CD;
    psp->memtop = 0x1FFF;
    psp->terminate_addr = MK_FP(0xFFFF, 0x0000);
    psp->ctrl_break_addr = MK_FP(0xFFFF, 0x0000);
    psp->critical_error_addr = MK_FP(0xFFFF, 0x0000);
    psp->argc = 0;
    psp->argv[0] = 13;
    psp->argv[1] = 0;

    outportb(0x60, 0x80);

    ia16_int_set_handler(0x22, (ia16_int_handler_t) MK_FP(0xFFFF, 0x0000));
    ia16_int_set_handler(0x23, (ia16_int_handler_t) MK_FP(0xFFFF, 0x0000));
    ia16_int_set_handler(0x24, (ia16_int_handler_t) MK_FP(0xFFFF, 0x0000));

    asm volatile("ljmp $0x1000,$0x0100\n");
    return 0;
}