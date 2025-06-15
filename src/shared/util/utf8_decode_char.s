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

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .fartext.s.utf8_decode_char, "ax"
    .global utf8_decode_char
utf8_decode_char:
    push ds
    push si

    // SS:BX = pointer to far pointer to string
    // DS:SI = far pointer to string
    mov bx, ax
    ss lds si,[bx]
    xor ax, ax
    mov dx, ax

    lodsb
    cmp al, 0xF0
    jae utf8_decode_char4
    cmp al, 0xE0
    jae utf8_decode_char3
    cmp al, 0xC0
    jae utf8_decode_char2
    cmp al, 0x80
    jae utf8_decode_char_err
utf8_decode_char_ret:
    ss mov [bx], si
    mov si, ds
    ss mov [bx + 2], si

    pop si
    pop ds

    IA16_RET

utf8_decode_char2:
    and al, 0x1F
    shl ax, 6
    mov cx, ax
    lodsb
    and ax, 0x003F
    or ax, cx
    jmp utf8_decode_char_ret

utf8_decode_char3:
    shl ax, 12
    mov cx, ax
    lodsb
    and ax, 0x003F
    shl ax, 6
    or cx, ax
    lodsb
    and ax, 0x003F
    or ax, cx
    jmp utf8_decode_char_ret

utf8_decode_char4:
    shl ax, 2
    and ax, 0x001C
    mov dx, ax
    lodsb
    and ax, 0x003F
    push ax
    shr ax, 4
    or dx, ax
    pop ax
    jmp utf8_decode_char3

utf8_decode_char_err:
    mov al, '?'
    jmp utf8_decode_char_ret
