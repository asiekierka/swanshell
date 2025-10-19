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

#include <stdlib.h>
#include "task.h"

__attribute__((stdcall))
void task_return(task_t *task);

#ifdef __IA16_CMODEL_IS_FAR_TEXT
#define TASK_STACK_RESERVE 6
#else
#define TASK_STACK_RESERVE 4
#endif

task_t *task_init(void *ptr, size_t len, task_func_t func) {
    len &= ~1;

    task_t *task = (task_t*) ptr;
    uint16_t stack_size = len - sizeof(task_t);
    uint16_t stack_end = stack_size >> 1;

    task->sp = (uint16_t) task + len - TASK_STACK_RESERVE;
    task->stack[stack_end - 1] = (uint16_t) task;
#ifdef __IA16_CMODEL_IS_FAR_TEXT
    task->stack[stack_end - 2] = FP_SEG(task_return);
    task->stack[stack_end - 3] = FP_OFF(task_return);
#else
    task->stack[stack_end - 2] = (uint16_t) task_return;
#endif
    task->jmp_ptr = func;

    return task;
}

task_t *task_allocate(uint16_t stack_size, task_func_t func) {
    size_t task_size = stack_size + TASK_STACK_RESERVE + sizeof(task_t);
    task_t *task = malloc(task_size);
    if (task != NULL)
        task_init(task, task_size, func);
    return task;
}

int task_join(task_t *task) {
    int retval = 0;
    while (!task_is_joined(task))
        retval = task_resume(task);
    return retval;
}