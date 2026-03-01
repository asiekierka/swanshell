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
#include <nile/fpga.h>
#include <nilefs.h>
#include <nilefs/ff.h>
#include <string.h>
#include "shell.h"
#include "cart/status.h"
#include "lang_gen.h"
#include "launch/launch_athena.h"
#include "strings.h"
#include "util/file.h"
#include "util/task/task.h"
#include "errors.h"
#include "lang.h"
#include "launch/launch.h"
#include "xmodem.h"

// Avoid shell stack memory overlapping with bootstub
__attribute__((section(".iram_1700")))
uint8_t shell_task_mem[1152];
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
DEFINE_STRING_LOCAL(s_dot_local, ".");
DEFINE_STRING_LOCAL(s_about, "about");
DEFINE_STRING_LOCAL(s_cd, "cd");
DEFINE_STRING_LOCAL(s_help, "help");
DEFINE_STRING_LOCAL(s_launch, "launch");
DEFINE_STRING_LOCAL(s_ls, "ls");
DEFINE_STRING_LOCAL(s_reboot, "reboot");
DEFINE_STRING_LOCAL(s_rm, "rm");
DEFINE_STRING_LOCAL(s_upload, "upload");
DEFINE_STRING_LOCAL(s_file_too_big, "File too big");
DEFINE_STRING_LOCAL(s_missing_argument, "Missing argument");
DEFINE_STRING_LOCAL(s_unknown_command, "Unknown command");
DEFINE_STRING_LOCAL(s_backspace, "\x08 \x08");
DEFINE_STRING_LOCAL(s_awaiting_xmodem_transfer, "Awaiting XMODEM transfer");
DEFINE_STRING_LOCAL(s_saving_file, "Saving file");
DEFINE_STRING_LOCAL(s_help_output,
"Commands:\n"
"about          \tAbout swanshell\n"
"cd <path>      \tChange current directory to specified path\n"
"help           \tPrint help information\n"
"launch [path]  \tLaunch file via XMODEM or via path\n"
"ls [path]      \tList files in path\n"
"reboot         \tSoft reboot cartridge\n"
"rm <path>      \tRemove file at path\n"
"upload <path>  \tUpload file to storage card via XMODEM\n"
);

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

static void shell_task_yield(uint16_t ret) {
    task_yield(shell_task, ret);
    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
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

static void shell_help(void) {
    nile_mcu_native_cdc_write_string(s_help_output);
}

static void shell_reboot(void) {
    ia16_disable_irq();
    nilefs_eject();
    nile_soft_reset();
}

__attribute__((noinline))
static void shell_print_error(int16_t result) {
    if (result == FR_OK) return;
    nile_mcu_native_cdc_write_string(error_to_string(result));
}

__attribute__((noinline))
static int16_t shell_launch_file(char *path) {
    const char *ext = (const char*) strrchr(path, '.');
    if (ext == NULL) return ERR_FILE_NOT_EXECUTABLE;
    if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc) || !strcasecmp(ext, s_file_ext_pc2)) {
        launch_rom_metadata_t meta;
        int16_t result = launch_get_rom_metadata(path, &meta);
        if (result == FR_OK) {
            result = launch_restore_save_data(path, &meta);
            if (result == ERR_MCU_COMM_FAILED) {
                shell_print_error(result);
                nile_mcu_native_cdc_write_string_const(s_new_line);
                result = FR_OK;
            }
            if (launch_is_battery_required(&meta)) {
                if (cart_status_mcu_info_valid() && !cart_status_mcu_battery_ok()) {
                    nile_mcu_native_cdc_write_string(lang_keys[LK_PROMPT_NO_BATTERY_TITLE]);
                    nile_mcu_native_cdc_write_string_const(s_new_line);
                }
            }
            if (result == FR_OK) {
                result = launch_set_bootstub_file_entry(path, &bootstub_data->prog);
                if (result == FR_OK) {
                    result = launch_rom_via_bootstub(&meta);
                }
            }
            shell_task_yield(SHELL_RET_REFRESH_UI);
        }
        return result;
    } else if (!strcasecmp(ext, s_file_ext_fx) || !strcasecmp(ext, s_file_ext_bin)) {
        // TODO: Do some checks on the .bin file
        return launch_athena_boot_curdir_as_rom_wip(path);
    } else if (!strcasecmp(ext, s_file_ext_bfb)) {
        int16_t result = launch_bfb(path);
        shell_task_yield(SHELL_RET_REFRESH_UI);
        return result;
    } else if (!strcasecmp(ext, s_file_ext_com)) {
        int16_t result = launch_com(path);
        shell_task_yield(SHELL_RET_REFRESH_UI);
        return result;
    } else {
        return ERR_FILE_NOT_EXECUTABLE;
    }
}

__attribute__((noinline))
static void shell_launch(void) {
    int16_t result = FR_OK;
    uint32_t size = 0;
    if (shell_flags & SHELL_FLAG_INTERACTIVE) {
        nile_mcu_native_cdc_write_string_const(s_awaiting_xmodem_transfer);
    }
    result = xmodem_recv_start(&size);
    nile_mcu_native_cdc_write_string_const(s_new_line);
    if (result == FR_OK) {
        bootstub_data->prog.size = size;
        shell_task_yield(SHELL_RET_LAUNCH_IN_PSRAM);
    }
    if (result != FR_OK) {
        shell_print_error(result);
    }
}

static void shell_file_callback(void *userdata, uint32_t step, uint32_t max) {
    nile_mcu_native_cdc_write_string_const(s_dot_local);
}

static void shell_upload(void) {
    if (shell_flags & SHELL_FLAG_INTERACTIVE) {
        nile_mcu_native_cdc_write_string_const(s_awaiting_xmodem_transfer);
    }
    uint32_t size = 0;
    int16_t result = xmodem_recv_start(&size);
    ws_delay_ms(10);
    nile_mcu_native_cdc_write_string_const(s_new_line);
    if (result == FR_OK) {
        FIL fp;
        nile_mcu_native_cdc_write_string_const(s_saving_file);
        result = f_open(&fp, shell_line + 7, FA_WRITE | FA_CREATE_ALWAYS);
        if (result == FR_OK) {
            result = f_write_rom_banked(&fp, 0, size, shell_file_callback, NULL);
            f_close(&fp);
        }
        nile_mcu_native_cdc_write_string_const(s_new_line);
    }
    if (result != FR_OK) {
        shell_print_error(result);
    }
    shell_task_yield(SHELL_RET_REFRESH_UI);
}

static void shell_cd(void) {
    int16_t result = f_chdir(shell_line + 3);
    if (result != FR_OK) {
        shell_print_error(result);
    }
    shell_task_yield(SHELL_RET_REFRESH_UI);
}

static void shell_ls(void) {
    DIR dp;
    FILINFO fno;
    int16_t result = f_opendir(&dp, shell_line + 3);
    if (result == FR_OK) {
        while (true) {
    		result = f_readdir(&dp, &fno);
    		if (result != FR_OK)
                break;
    		if (fno.fname[0] == 0)
    			break;
            nile_mcu_native_cdc_write_string(fno.fname);
            nile_mcu_native_cdc_write_string_const(s_new_line);
    	}

        f_closedir(&dp);
    }
    if (result != FR_OK) {
        shell_print_error(result);
    }
}

static void shell_rm(void) {
    int16_t result = f_unlink(shell_line + 3);
    if (result != FR_OK) {
        shell_print_error(result);
    }
    shell_task_yield(SHELL_RET_REFRESH_UI);
}

int shell_func(task_t *task) {
    while (true) {
        shell_task_yield(SHELL_RET_IDLE);
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
                    } else if (!memcmp(shell_line, s_cd, 2)) {
                        if (shell_line_pos <= 3 || shell_line[2] != ' ') {
                            nile_mcu_native_cdc_write_string_const(s_missing_argument);
                        } else {
                            shell_cd();
                        }
                    } else if (!memcmp(shell_line, s_rm, 2)) {
                        if (shell_line_pos <= 3 || shell_line[2] != ' ') {
                            nile_mcu_native_cdc_write_string_const(s_missing_argument);
                        } else {
                            shell_rm();
                        }
                    } else if (!strcmp(shell_line, s_help)) {
                        shell_help();
                    } else if (!strcmp(shell_line, s_reboot)) {
                        shell_reboot();
                    } else if (!memcmp(shell_line, s_ls, 2)) {
                        if (shell_line_pos <= 3 || shell_line[2] != ' ') {
                            strcpy(shell_line + 3, s_dot_local);
                        }
                        shell_ls();
                    } else if (!memcmp(shell_line, s_launch, 6)) {
                        if (shell_line_pos <= 7 || shell_line[6] != ' ') {
                            shell_launch();
                        } else{
                            shell_print_error(shell_launch_file(shell_line + 7));
                        }
                    } else if (!memcmp(shell_line, s_upload, 6)) {
                        if (shell_line_pos <= 7 || shell_line[6] != ' ') {
                            nile_mcu_native_cdc_write_string_const(s_missing_argument);
                        } else {
                            shell_upload();
                        }
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
    }
}

void shell_init(void) {
    shell_line_pos = 0;
    shell_flags = 0;
    task_init(shell_task_mem, sizeof(shell_task_mem), shell_func);
}

bool shell_tick(void) {
    if (shell_flags == SHELL_FLAG_NOT_INITIALIZED) return false;
    if (task_is_joined(shell_task)) return false;

    switch (task_resume(shell_task)) {
        case SHELL_RET_REFRESH_UI: return true;
        case SHELL_RET_LAUNCH_IN_PSRAM: {
            launch_in_psram(bootstub_data->prog.size);
        } return false;
        default: return false;
    }
}
