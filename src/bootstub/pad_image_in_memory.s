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

#include <wonderful.h>
#include <ws.h>

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .text, "ax"
    // dx:ax = final byte of ROM
    // cx = final bank of ROM
    .global pad_image_in_memory
pad_image_in_memory:
    pusha
    push ds
    push es

    push 0x2000
    pop ds
    push 0x1000
    pop es

    // copying bytes from DX (bank):SI (offset) to BP (bank):DI (offset)
    mov bp, cx
    mov si, ax
    mov di, 0xFFFF
    std

    mov ax, dx
    out WS_CART_EXTBANK_ROM0_PORT, ax
    mov ax, bp
    out WS_CART_EXTBANK_RAM_PORT, ax

1:
    // CX = number of bytes to transfer at once
    mov cx, si
    cmp cx, di
    jb 2f
    mov cx, di

2:
    movsb
    rep movsb
    cmp si, 0xFFFF
    jne 2f

    test dx, dx
    jz 9f

    dec dx
    mov ax, dx
    out WS_CART_EXTBANK_ROM0_PORT, ax

2:
    cmp di, 0xFFFF
    jne 1b

    dec bp
    mov ax, bp
    out WS_CART_EXTBANK_RAM_PORT, ax

    push ss
    pop ds
    IA16_CALL progress_tick
    push 0x2000
    pop ds

    jmp 1b

9:
    pop es
    pop ds
    popa
    IA16_RET
