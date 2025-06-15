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

#define BITMAP_T_CURRENT_PITCH 0

    .section .data.s.bitmap, "a"
per_row_offset:
    .byte 0

    .section .fartext.s.bitmap, "ax"
    // Perform an operation, given a bitmap row r:
    // r = (r & ~mask) | (r & and ^ xor) & mask
    // BX = and
    // CX = row count
    // DX = xor
    // BP = mask
    // DS:SI = bitmap data
    // DS:DI = bitmap location
    // clobbers AX, CX, SI
__bitmap_bitop_row:
__bitmap_bitop_row_loop:
    // r = (r & ~mask) | (r & and ^ xor) & mask
    mov ax, [si]
    and ax, bx
    xor ax, dx
    and ax, bp
    push bp
    not bp
    and bp, [si]
    or ax, bp
    pop bp
    mov [si], ax
    // next row
    add si, [di + BITMAP_T_CURRENT_PITCH]
    loop __bitmap_bitop_row_loop
    ret

    /* 
    // left border
    if (x & 7) {
        uint16_t next_x = (x + 7) & (~7);
        uint16_t left_width = next_x - x;
        if (left_width > width) {
            left_width = width;
        }
        uint8_t left_offset = 8 - (x & 7) - left_width;

        uint8_t *vtile = tile;
        uint8_t mask = ~(count_width_mask[left_width] << left_offset);
        for (uint16_t i = 0; i < height * bitmap->bpp; i++) {
            *(vtile++) &= mask;
        }

        x += left_width;
        width -= left_width;
    }

    // center
    while (width >= 8) {
        memset(tile, 0, height * bitmap->bpp);
        tile += bitmap->x_pitch;
        x += 8;
        width -= 8;    
    }

    // right border
    if (width) {
        uint8_t *vtile = tile;
        uint8_t mask = ~(count_width_mask[width] << (7 - width));
        for (uint16_t i = 0; i < height * bitmap->bpp; i++) {
            *(vtile++) &= mask;
        }
    } */

    // __bitmap_bitop_fill_c(uint16_t value, void *dest, uint16_t _rows)
    .global __bitmap_bitop_fill_c
__bitmap_bitop_fill_c:
    cld
    push es
    push ss
    pop es
    push di
    mov di, dx
    rep stosw
    pop di
    pop es
    IA16_RET

    // __bitmap_bitop_row_c(uint16_t _and, uint16_t _xor, uint16_t _rows, uint16_t _mask, bitmap_t *bitmap, void *dest)
    .global __bitmap_bitop_row_c
__bitmap_bitop_row_c:
    mov bx, ax
    push ds
    push ss
    pop ds
    push si
    push di
    push bp
    mov bp, sp
    mov si, [bp + IA16_CALL_STACK_OFFSET(12)]
    mov di, [bp + IA16_CALL_STACK_OFFSET(10)]
    mov bp, [bp + IA16_CALL_STACK_OFFSET(8)]
    call __bitmap_bitop_row
    pop bp
    pop di
    pop si
    pop ds
    IA16_RET 0x6

__bitmap_c2p_4bpp_table:
#include "c2p_table.inc"

    // uint32_t bitmap_c2p_4bpp_pixel(uint32_t v)
    // DX:BX => CX:AX
    .global bitmap_c2p_4bpp_pixel
bitmap_c2p_4bpp_pixel:
    push bp
    push ds
    push cs
    pop ds
    mov bp, 4
    mov bx, ax
    xor ax, ax
    mov cx, ax
    jmp __bitmap_c2p_4bpp_loop_start
__bitmap_c2p_4bpp_loop:
    shl cx, 1
    shl ax, 1
    shl cx, 1
    shl ax, 1
__bitmap_c2p_4bpp_loop_start:
    push bx
    mov bh, 0
    shl bx, 1
    shl bx, 1
    or ax, [bx+__bitmap_c2p_4bpp_table]
    or cx, [bx+__bitmap_c2p_4bpp_table+2]
    pop bx
    dec bp
    jz __bitmap_c2p_4bpp_done
    mov bl, bh
    mov bh, dl
    mov dl, dh
    jmp __bitmap_c2p_4bpp_loop
__bitmap_c2p_4bpp_done:
    pop ds
    pop bp
    mov dx, cx
    IA16_RET
