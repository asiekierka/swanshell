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
    .global cold_jump

    // DX:AX - jump pointer
    // CX - bank limit
    .section .fartext.s.cold_jump, "a"
    .align 2
cold_jump:
    // Patch address in jump stub
    mov [launch_ram_asm_relocated_stub + 1], ax
    mov [launch_ram_asm_relocated_stub + 3], dx

    // Write bank values
    mov ax, 0xFFFF
    out WS_CART_BANK_ROML_PORT, al
    out WS_CART_EXTBANK_RAM_PORT, ax
    out WS_CART_EXTBANK_ROM0_PORT, ax
    out WS_CART_EXTBANK_ROM1_PORT, ax

    // Clear DS/ES for memory I/O
    cld
    inc ax
    mov ds, ax
    mov es, ax

    // Copy stub to RAM
    mov si, offset "launch_ram_asm_relocated"
    mov di, 0x2000
    // Populate immediate values in jump stub
    mov ax, [0x40]
    stosw
    mov ax, [0x50]
    stosw
    mov cx, (launch_ram_asm_relocated_end + 1 - launch_ram_asm_relocated)
    shr cx, 1
    rep movsw

    // Clear 0x00 - 0x3F
    xor ax, ax
    mov di, ax
    mov cx, (0x40 >> 1)
    rep stosw

    // Clear 0x58 - 0x1FFF and jump to relocated stub
    mov di, 0x58
    mov cx, ((0x2000 - 0x58) >> 1)
    jmp 0x2004

launch_ram_asm_relocated:
    // Clear unused memory
    rep stosw

    // Restore (most) register state
    mov cx, [0x56]
    push cx
    popf
    push ax // Clear stack area we just used
    mov     bx, [0x42]
    mov     cx, [0x44]
    mov     dx, [0x46]
    mov     sp, [0x48]
    mov     bp, [0x4A]
    mov     si, [0x4C]
    mov     di, [0x4E]
    mov     es, [0x52]
    mov     ss, [0x54]

    // Clear register restore area
    mov ax, 0x0000
    mov [0x40], ax
    mov [0x42], ax
    mov [0x44], ax
    mov [0x46], ax
    mov [0x48], ax
    mov [0x4A], ax
    mov [0x4C], ax
    mov [0x4E], ax
    mov [0x50], ax
    mov [0x52], ax
    mov [0x54], ax
    mov [0x56], ax

    // Restore final registers
    mov ax, [0x2002]
    mov ds, ax
    ss mov ax, [0x2000]

    // Fly me to the moon
launch_ram_asm_relocated_stub:
    jmp 0xffff,0x0000
launch_ram_asm_relocated_end:
