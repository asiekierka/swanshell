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
    // cx = count, decrement if negative
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
    mov ax, [bp + IA16_CALL_STACK_OFFSET(8)]

    cld
    cmp byte ptr [is_vertical], 0
    jnz memcpy_expand_8_16_dec
1:
    lodsb
    stosw
    loop 1b

2:
    pop bp
    pop di
    pop si
    pop es
    IA16_RET 0x2

memcpy_expand_8_16_dec:
    add di, cx
    add di, cx
1:
    sub di, 2
    lodsb
    mov [di], ax
    loop 1b
    jmp 2b
