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
    .global xmodem_checksum
xmodem_checksum:
    push si

    mov si, ax
    mov al, 0x00
    mov cx, 16

1:
    add al, [si]
    add al, [si+1]
    add al, [si+2]
    add al, [si+3]
    add al, [si+4]
    add al, [si+5]
    add al, [si+6]
    add al, [si+7]
    add si, 8
    loop 1b

    pop si
    IA16_RET
