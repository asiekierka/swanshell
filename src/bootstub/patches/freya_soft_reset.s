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

    .section .text.s.freya_soft_reset, "ax"
    .global freya_soft_reset
    .global freya_soft_reset_end
freya_soft_reset:
    // unlock nileswan registers
    mov al, 0xDD
    out 0xE2, al

    // reset linear ROM bank
    mov ax, 0xFFFF
    out 0xC0, al
    // hide display
    push ax
    inc ax
    out 0x00, ax
    push ax
    dec ax

    // jump to 0xFFFF:0x0000
    // this code takes advantage of V30MZ prefetch
    out 0xE4, ax
    retf
freya_soft_reset_end:
