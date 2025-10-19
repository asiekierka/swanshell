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

#ifdef __IA16_CMODEL_IS_FAR_TEXT
    .section .fartext.libtask, "ax"
#else
    .section .text.libtask, "ax"
#endif

    // Swap registers with task structure
ctx_swap:
    pop dx
    xchg sp, [bx]
    xchg bp, [bx + 2]
    xchg si, [bx + 4]
    xchg di, [bx + 6]
    mov cx, es
    xchg cx, [bx + 8]
    mov es, cx
    push dx
    ret

    .align 2
    .global task_resume
task_resume:
    mov bx, ax
    call ctx_swap
    // Jump to task IP
#ifdef __IA16_CMODEL_IS_FAR_TEXT
    ljmp [bx + 10]
#else
    jmp [bx + 10]
#endif

    .align 2
    .global task_yield
task_yield:
    pop ax // IP
#ifdef __IA16_CMODEL_IS_FAR_TEXT
    pop dx // CS
#endif
    pop bx // task_t pointer
    // Store new task IP
    mov [bx + 10], ax
#ifdef __IA16_CMODEL_IS_FAR_TEXT
    mov [bx + 12], dx
#endif
    pop ax // return value
    call ctx_swap
    IA16_RET

    .align 2
    .global task_return
task_return:
    pop bx // task_t pointer
    call ctx_swap

    // Clear task IP
#ifdef __IA16_CMODEL_IS_FAR_TEXT
    xor cx, cx
    mov [bx + 10], cx
    mov [bx + 12], cx
#else
    mov word ptr [bx + 10], 0
#endif
    IA16_RET