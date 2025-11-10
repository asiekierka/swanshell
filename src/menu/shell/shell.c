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
#include <nilefs.h>
#include <string.h>
#include "shell.h"
#include "strings.h"
#include "util/task/task.h"
#include "errors.h"
#include "lang.h"
#include "launch/launch.h"
#include "xmodem.h"

uint8_t shell_task_mem[512];
#define shell_task ((task_t*) shell_task_mem)

#define SHELL_LINE_LENGTH 128
char shell_line[SHELL_LINE_LENGTH + 1];
uint8_t shell_line_pos;

#define SHELL_FLAG_NOT_INITIALIZED 0xFF
#define SHELL_FLAG_INTERACTIVE 0x01
uint8_t shell_flags = SHELL_FLAG_NOT_INITIALIZED;

DEFINE_STRING_LOCAL(s_line_too_long, "\r\nLine too long");
DEFINE_STRING_LOCAL(s_new_line, "\r\n");
DEFINE_STRING_LOCAL(s_new_prompt, "\r\n> ");
DEFINE_STRING_LOCAL(s_about, "about");
DEFINE_STRING_LOCAL(s_launch, "launch");
DEFINE_STRING_LOCAL(s_unknown_command, "Unknown command");
DEFINE_STRING_LOCAL(s_backspace, "\x08 \x08");
DEFINE_STRING_LOCAL(s_awaiting_xmodem_transfer, "Awaiting XMODEM transfer");

static const char __far s_version_suffix[] = " " VERSION;

#define nile_mcu_native_cdc_write_string_const(s) nile_mcu_native_cdc_write_sync(s, sizeof(s)-1)
static void nile_mcu_native_cdc_write_string(const char __far* s) {
    int last_pos = 0;
    int pos = 0;
    while (true) {
        if (s[pos] == 0 || s[pos] == 10) {
            if (last_pos != pos) {
                nile_mcu_native_cdc_write_sync(s + last_pos, pos - last_pos);
            }
            last_pos = pos + 1;
            if (s[pos] == 10) {
                nile_mcu_native_cdc_write_string_const(s_new_line);
            } else {
                break;
            }
        }
        pos++;
    }
}

static void shell_new_prompt(void) {
    shell_line_pos = 0;
    nile_mcu_native_cdc_write_string(shell_flags & SHELL_FLAG_INTERACTIVE ? s_new_prompt : s_new_line);
}

static void shell_about(void) {
    nile_mcu_native_cdc_write_string(lang_keys_en[LK_NAME]);
    nile_mcu_native_cdc_write_string_const(s_version_suffix);
    nile_mcu_native_cdc_write_string_const(s_new_line);

    nile_mcu_native_cdc_write_string(lang_keys_en[LK_NAME_COPYRIGHT]);
    nile_mcu_native_cdc_write_string_const(s_new_line);
    nile_mcu_native_cdc_write_string_const(s_new_line);

    nile_mcu_native_cdc_write_string(lang_keys_en[LK_NAME_COPYRIGHT_INFO]);
}

static void shell_launch(void) {
    // TODO: Support argument

    if (shell_flags & SHELL_FLAG_INTERACTIVE) {
        nile_mcu_native_cdc_write_string_const(s_awaiting_xmodem_transfer);
    }
    uint32_t size = 0;
    int16_t result = xmodem_recv_start(&size);
    if (result == FR_OK) {
        bootstub_data->prog.size = size;
        task_yield(shell_task, SHELL_RET_LAUNCH_IN_PSRAM);
    }
    if (result != FR_OK) {
        nile_mcu_native_cdc_write_string_const(s_new_line);
        nile_mcu_native_cdc_write_string(error_to_string(result));
    }
}

int shell_func(task_t *task) {
    while (true) {
        int bytes_read = nile_mcu_native_cdc_read_sync(shell_line + shell_line_pos, 1);
        if (bytes_read > 0) {
            char c = shell_line[shell_line_pos];
            if (c == 13) {
                if (shell_line_pos > 0) {
                    // ENTER pressed, parse line
                    shell_line[shell_line_pos] = 0;
                    nile_mcu_native_cdc_write_string_const(s_new_line);

                    if (!strcmp(shell_line, s_about)) {
                        shell_about();
                    } else if (!strcmp(shell_line, s_launch)) {
                        shell_launch();
                    } else {
                        nile_mcu_native_cdc_write_string_const(s_unknown_command);
                    }
                } else {
                    shell_flags |= SHELL_FLAG_INTERACTIVE;
                }

                shell_new_prompt();
            } else if (c == 8) {
                // Backspace
                if (shell_line_pos > 0) {
                    if (shell_flags & SHELL_FLAG_INTERACTIVE) {
                        nile_mcu_native_cdc_write_string_const(s_backspace);
                    }
                    shell_line_pos--;
                }
            } else if (c >= 32 && c <= 126) {
                // Echo byte back
                if (shell_flags & SHELL_FLAG_INTERACTIVE) {
                    nile_mcu_native_cdc_write_sync(shell_line + shell_line_pos, 1);
                }

                if (shell_line_pos == SHELL_LINE_LENGTH) {
                    // line too long
                    nile_mcu_native_cdc_write_string_const(s_line_too_long);
                    shell_new_prompt();
                } else {
                    shell_line_pos++;
                }
            }
        }

        task_yield(task, SHELL_RET_IDLE);
    }
}

void shell_init(void) {
    shell_line_pos = 0;
    shell_flags = 0;
    task_init(shell_task_mem, sizeof(shell_task_mem), shell_func);
}

void shell_tick(void) {
    if (shell_flags == SHELL_FLAG_NOT_INITIALIZED) return;
    if (task_is_joined(shell_task)) return;

    switch (task_resume(shell_task)) {
        case SHELL_RET_LAUNCH_IN_PSRAM: {
            launch_in_psram(bootstub_data->prog.size);
        } break;
    }
}
