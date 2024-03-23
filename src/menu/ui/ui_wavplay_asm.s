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

#define WAV_BUFFER_LINEAR0 0x06000
#define POSITION_COUNTER_INCR_LOW 0x18
#define POSITION_COUNTER_INCR_HIGH 0x1A
#define POSITION_COUNTER_LOW 0x1C
#define POSITION_COUNTER_HIGH 0x1E

    .section .fartext.s.ui_wavplay_asm, "ax"
    .global ui_wavplay_irq_8_mono
ui_wavplay_irq_8_mono:
	// read next audio byte
	push ax
	push bx
	push ds

	mov bx, 0x0000
	mov ds, bx
	
	mov ax, word ptr [POSITION_COUNTER_LOW]
	add ax, word ptr [POSITION_COUNTER_INCR_LOW]
	mov word ptr [POSITION_COUNTER_LOW], ax

	mov ax, word ptr [POSITION_COUNTER_HIGH]
	mov bx, ax
	adc ax, word ptr [POSITION_COUNTER_INCR_HIGH]
	mov word ptr [POSITION_COUNTER_HIGH], ax
	shr bx, 2
	add bx, WAV_BUFFER_LINEAR0
	mov al, byte ptr [bx]
	out 0x89, al

	mov al, 0x80
	out 0xB6, al

    pop ds
	pop bx
	pop ax
	iret
