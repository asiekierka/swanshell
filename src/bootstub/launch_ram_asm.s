/**
 * Copyright (c) 2024, 2026 Adrian Siekierka
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

#define COLD_JUMP_PART2_IRAM_OFFSET 0x2004
#define COLD_JUMP_IPC_IRAM_OFFSET 0xE0
#define COLD_JUMP_IPC_IRAM_RUN_OFFSET ((COLD_JUMP_IPC_IRAM_OFFSET) + 0x18)

    // DX:AX - jump pointer
    .align 2
    .global cold_jump_via_iram
cold_jump_via_iram:
    // Patch address in jump stub
    mov [cold_jump_part2_iram_stub + 1], ax
    mov [cold_jump_part2_iram_stub + 3], dx

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
    mov si, offset "cold_jump_part2_iram"
    mov di, COLD_JUMP_PART2_IRAM_OFFSET
    // Populate immediate values in jump stub
    mov ax, [0x40]
    stosw
    mov ax, [0x50]
    stosw
    mov cx, (cold_jump_part2_iram_end + 1 - cold_jump_part2_iram)
    shr cx, 1
    rep movsw

    // Clear 0x00 - 0x3F
    xor ax, ax
    mov di, ax
    mov cx, (0x40 >> 1)
    rep stosw

    // Clear 0x58 - 0x1FFF and jump to relocated stub
    mov di, 0x58
    mov cx, ((COLD_JUMP_PART2_IRAM_OFFSET - 0x58) >> 1)
    jmp (COLD_JUMP_PART2_IRAM_OFFSET + 4)

cold_jump_part2_iram:
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
    mov ax, [COLD_JUMP_PART2_IRAM_OFFSET + 2]
    mov ds, ax
    ss mov ax, [COLD_JUMP_PART2_IRAM_OFFSET]

    // Fly me to the moon
cold_jump_part2_iram_stub:
    jmp 0xffff,0x0000
cold_jump_part2_iram_end:

    // DX:AX - jump pointer
    .align 2
    .global cold_jump_via_ipc
cold_jump_via_ipc:
    push ax

    // Write bank values
    mov ax, 0x000E
    out WS_CART_EXTBANK_RAM_PORT, ax
    mov ax, 0xFFFF
    out WS_CART_BANK_ROML_PORT, al
    out WS_CART_EXTBANK_ROM0_PORT, ax
    out WS_CART_EXTBANK_ROM1_PORT, ax

    // Copy part2 to IPC
    cld
    inc ax
    mov ds, ax
    push 0x1000
    pop es

    // Copy register state to IPC
    mov si, 0x40
    mov di, COLD_JUMP_IPC_IRAM_OFFSET
    mov cx, (0x18 >> 1)
    rep movsw

    // Copy part2 to IPC
    mov si, offset "cold_jump_part2_ipc"
    mov cx, (cold_jump_part2_ipc_end + 1 - cold_jump_part2_ipc)
    shr cx, 1
    rep movsw

    pop ax
    jmp 0x1000, COLD_JUMP_IPC_IRAM_RUN_OFFSET

cold_jump_part2_ipc:
    // Write jump stub to 0x0400
    mov word ptr [0x400], 0xD0E7 // OUT 0xD0, AX
    mov byte ptr [0x402], 0xB8 // MOV AX, nnnn
    mov cx, [0x40]
    mov word ptr [0x403], cx
    mov byte ptr [0x405], 0xEA // JMP dx:ax
    mov word ptr [0x406], ax
    mov word ptr [0x408], dx

    push ds
    push es
    pop ds
    pop es // DS = 0x1000, ES = 0x0000

    // Clear 0x0000 - 0x03FF
    xor ax, ax
    mov di, ax
    mov cx, (0x400 >> 1)
    rep stosw

    // Clear 0x040A - 0x1FFF
    mov di, 0x40A
    mov cx, ((0x2000 - 0x40A) >> 1)
    rep stosw

    // Restore register state
    mov cx, [0xF6]
    push cx
    popf
    push ax // Clear stack area we just used
    mov     bx, [0xE2]
    mov     cx, [0xE4]
    mov     dx, [0xE6]
    mov     sp, [0xE8]
    mov     bp, [0xEA]
    mov     si, [0xEC]
    mov     di, [0xEE]
    mov     es, [0xF2]
    mov     ss, [0xF4]
    mov     ds, [0xF0]
    
    mov     ax, 0xFFFF
    jmp 0x0000,0x0400
cold_jump_part2_ipc_end:
