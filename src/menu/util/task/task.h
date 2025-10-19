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
 
#ifndef TASK_H_
#define TASK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wonderful.h>

typedef struct {
    uint16_t sp, bp, si, di, es;
#ifdef __IA16_CMODEL_IS_FAR_TEXT
    void __far* jmp_ptr;
#else
    void * jmp_ptr;
#endif
    uint16_t stack[];
} task_t;

/**
 * @brief Task function.
 * 
 * The argument is the task instance pointer (used for task_yield()).
 * The return value is passed via task_resume().
 */
typedef int (*task_func_t)(task_t *task);

/**
 * @brief Allocate a new task instance on dynamic heap memory.
 * 
 * @param stack_size The size of the stack.
 * @param func The function to call inside the task.
 * @return task_t* The pointer to the task instance.
 */
task_t *task_allocate(uint16_t stack_size, task_func_t func);

/**
 * @brief Join a task, looping indefinitely until it completes.
 * 
 * @param task Task instance pointer.
 * @return int Return value of the task.
 */
int task_join(task_t *task);

/**
 * @brief Resume a task, returning once it yields.
 * 
 * @param task Task instance pointer.
 * @return int Return value of the task.
 */
int task_resume(task_t *task);

/**
 * @brief Free an instance allocated with task_allocate().
 * 
 * @param task Task instance pointer.
 */
static inline void task_free(task_t *task) {
    free(task);
}

/**
 * @brief Yield from inside a task instance.
 *
 * Do not call outside of a task instance!
 *
 * @param task Task instance pointer.
 * @param value Return value.
 */
__attribute__((stdcall))
void task_yield(task_t *task, int value);

/**
 * @brief Query if a task has been joined (completed).
 * 
 * @param task Task instance pointer.
 */
static inline bool task_is_joined(task_t *task) {
    return task->jmp_ptr == NULL;
}

#endif /* UTIL_H_ */
