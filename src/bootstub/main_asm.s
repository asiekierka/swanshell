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
 
#include <wonderful.h>
#include <ws.h>
#include <nile.h>

    .arch   i186
    .code16
    .intel_syntax noprefix
    .global restore_cold_boot_io_state

    .align 2
__outsw_loop:
    outsw
    inc dx
    inc dx
    loop __outsw_loop
    ret

    .section .fartext.s.restore_cold_boot_io_state, "a"
    .align 2
restore_cold_boot_io_state:
    push ds
    push es
    push si
    push di
    push ds
    pop es

    cld
    // clear 0x2000 - 0x3FFF (=> 0x00)
    mov di, 0x2000
    mov cx, (0x2000 >> 1)
    in al, 0x60
    mov bl, al
    xor ax, ax
    test bl, 0x80 // color?
    jz 1f
    // clear 0x2000 - 0xFDFF
    mov cx, (0xDE00 >> 1)
    rep stosw
    // clear 0xFE00 - 0xFFFF (=> 0xFF)
    mov cx, (0x200 >> 1)
    mov ax, 0xFFFF
1:
    rep stosw

    // reset I/O ports 0x00 - 0x5F
    mov ax, 0x1000
    mov ds, ax
    mov si, 0x0020
    xor dx, dx
    mov cx, (0x60 >> 1)
    call __outsw_loop

    // reset I/O ports 0x80 - 0x9F
    mov si, 0x0020 + 0x80
    mov dx, 0x80
    mov cx, (0x20 >> 1)
    call __outsw_loop

    // reset I/O ports 0xA2, 0xA3
    mov si, 0x0020 + 0xA2
    mov dx, 0xA2
    outsw

    // manually reset I/O ports 0xA2 - 0xAA
    // On system start, the timer reload values have non-deterministic readout
    // values; however, the counters act as if they were set to zero.
    // Maintaining this behaviour is more important.
    xor ax, ax
    out 0xA4, ax
    out 0xA6, ax

    // reset remaining I/O ports by hand
    // xor ax, ax
    out 0xB2, al
    out 0xB3, al
    out 0xB7, al
    mov al, 0x40
    out 0xB5, al

    pop di
    pop si
    pop es
    pop ds
    IA16_RET
