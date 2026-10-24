#include "launch/launch_router.h"
#include <string.h>
#include <wonderful.h>
#include <nilefs.h>
#include "cart/mcu.h"
#include "cart/status.h"
#include "lang.h"
#include "launch.h"
#include "../errors.h"
#include "../strings.h"
#include "launch/launch_athena.h"
#include "plugin/plugin.h"
#include "shell/shell.h"

int16_t launch_router_file(char *path, const char *ext, uint16_t flags) {
    int16_t code = FR_OK;

    if (ext != NULL) {
        if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc) || !strcasecmp(ext, s_file_ext_pc2)) {
            launch_rom_metadata_t meta;
            int16_t result = launch_get_rom_metadata(path, &meta);
            if (result == FR_OK) {
                result = launch_restore_save_data(path, &meta);
                if (result == ERR_MCU_COMM_FAILED) {
                    if (flags & LAUNCH_ROUTER_USE_SERIAL_OUT) {
                        shell_print_error(result);
                        shell_print_newline();
                        result = FR_OK;
                    } else {
                        if (launch_ui_handle_mcu_comm_error(&meta))
                            result = FR_OK;
                        else
                            goto exit_no_launch;
                    }
                } else {
                    if (launch_is_battery_required(&meta)) {
                        if (cart_status_mcu_info_valid() && !cart_status_mcu_battery_ok()) {
                            if (flags & LAUNCH_ROUTER_USE_SERIAL_OUT) {
                                nile_mcu_native_cdc_write_string(lang_keys[LK_PROMPT_NO_BATTERY_TITLE]);
                                shell_print_newline();
                            } else {
                                if (!launch_ui_handle_battery_missing_error(&meta))
                                    goto exit_no_launch;
                            }
                        }
                    }
                }
                if (result == FR_OK) {
                    result = launch_set_bootstub_file_entry(path, &bootstub_data->prog);
                    if (result == FR_OK) {
                        result = launch_rom_via_bootstub(&meta);
                    }
                }
            }

exit_no_launch:
            mcu_reset_if_not_native();
            return result;
        } else if (!strcasecmp(ext, s_file_ext_fx) || !strcasecmp(ext, s_file_ext_bin)) {
            // TODO: Do some checks on the .bin file
            return launch_athena_boot_curdir_as_rom_wip(path);
        } else if (!strcasecmp(ext, s_file_ext_vgm) || !strcasecmp(ext, s_file_ext_vgz)) {
            if (flags & LAUNCH_ROUTER_USE_SERIAL_OUT) return ERR_FILE_NOT_EXECUTABLE;
            code = ui_vgmplay(path);
        } else if (!strcasecmp(ext, s_file_ext_wav)) {
            if (flags & LAUNCH_ROUTER_USE_SERIAL_OUT) return ERR_FILE_NOT_EXECUTABLE;
            code = ui_wavplay(path);
        } else if (!strcasecmp(ext, s_file_ext_bmp)) {
            // SRAM corrupted by this operation
            if (flags & LAUNCH_ROUTER_USE_SERIAL_OUT) return ERR_FILE_NOT_EXECUTABLE;
            return ui_bmpview(path);
        } else if (!strcasecmp(ext, s_file_ext_bfb)) {
            return launch_bfb(path);
        } else if (!strcasecmp(ext, s_file_ext_com)) {
            return launch_com(path);
        } else if (!strcasecmp(ext, s_file_ext_rom)) {
            return launch_plugin_via_ipc(s_path_plugin_uxn, path);
        } else {
            goto txtview;
        }
    } else {
txtview:
        if (flags & LAUNCH_ROUTER_USE_SERIAL_OUT) {
            return ERR_FILE_NOT_EXECUTABLE;
        } else {
            code = ui_txtview(path);
        }
    }

    return !code ? LAUNCH_ROUTER_OK_SRAM_UNCHANGED : code;
}
