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

#include <nile.h>
#include "shell.h"
#include "util/task/task.h"

uint8_t shell_task_mem[512];
#define shell_task ((task_t*) shell_task_mem)

int shell_func(task_t *task) {
    // TODO

    /* uint8_t buffer[1];

    while (true) {
        int bytes_read = nile_mcu_native_cdc_read_sync(buffer, 1);
        if (bytes_read > 0)
            nile_mcu_native_cdc_write_sync(buffer, 1);
        
        task_yield(task, 0);
    } */
    return 0;
}

void shell_init(void) {
    task_init(shell_task_mem, sizeof(shell_task_mem), shell_func);
}

void shell_tick(void) {
    if (!task_is_joined(shell_task))
        task_resume(shell_task);
}