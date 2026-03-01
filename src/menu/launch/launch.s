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

#include <wonderful.h>
#include <ws.h>

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .fartext.s.launch, "ax"
    .global launch_jump_to_bootstub
launch_jump_to_bootstub:
    // Load bootstub to 0x00C0 and jump. Stack not trustworthy here!
    mov cx, ax

    in al, 0x60
    test al, 0x80
    jnz launch_jump_to_bootstub_gdma

    shr cx, 1
    // CX = bootstub size in words

    mov ax, 0x2000
    mov ds, ax
    xor ax, ax
    mov es, ax
    mov si, offset bootstub
    mov di, 0x00c0

    cld
    rep movsw

    jmp 0x0000,0x00c0

launch_jump_to_bootstub_gdma:
    mov ax, cx
    out 0x46, ax
    mov ax, offset bootstub
    out 0x40, ax
    mov ax, 0x0002
    out 0x42, al
    mov al, 0xc0
    out 0x44, ax
    mov al, 0x80
    out 0x48, al
    jmp 0x0000,0x00c0
