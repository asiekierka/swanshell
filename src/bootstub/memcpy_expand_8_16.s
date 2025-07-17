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

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .text, "ax"
    // ax = destination
    // dx = source
    // cx = count
    // stack = fill value
    .global memcpy_expand_8_16
memcpy_expand_8_16:
    push es
    push ds
    pop es
    push si
    push di
    push bp
    mov bp, sp

    mov di, ax
    mov si, dx
    mov ax, [IA16_CALL_STACK_OFFSET(10)]

    cld
memcpy_expand_8_16_loop:
    lodsb
    stosw
    loop memcpy_expand_8_16_loop

    pop bp
    pop di
    pop si
    pop es
    IA16_RET 0x2
