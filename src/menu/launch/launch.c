/**
 * Copyright (c) 2024, 2025, 2026 Adrian Siekierka
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

#include <nilefs/ff.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws.h>
#include <wsx/zx0.h>
#include <nile.h>
#include <nilefs.h>
#include "launch.h"
#include "bootstub.h"
#include "errors.h"
#include "lang_gen.h"
#include "cart/mcu.h"
#include "launch/launch_athena.h"
#include "settings.h"
#include "strings.h"
#include "../../build/menu/build/bootstub_bin.h"
#include "../../build/menu/assets/menu/bootstub_tiles.h"
#include "lang.h"
#include "ui/ui.h"
#include "ui/ui_popup_dialog.h"
#include "util/file.h"
#include "util/ini.h"

__attribute__((section(".iramCx_c000")))
uint8_t sector_buffer[CONFIG_MEMLAYOUT_SECTOR_BUFFER_SIZE];

extern FATFS fs;

#define NILE_IPC_SAVE_ID ((volatile uint32_t __far*) MK_FP(0x1000, 512 - sizeof(uint32_t)))

uint32_t launch_get_save_id(uint16_t target) {
    uint32_t save_id = SAVE_ID_NONE;

    if (target & SAVE_ID_FOR_FLASH) {
        uint8_t prev_cart_flash = inportb(WS_CART_BANK_FLASH_PORT);
        uint16_t prev_sram_bank = inportw(WS_CART_EXTBANK_RAM_PORT);
        outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
        outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
        save_id = *NILE_IPC_SAVE_ID;
        outportw(WS_CART_EXTBANK_RAM_PORT, prev_sram_bank);
        outportb(WS_CART_BANK_FLASH_PORT, prev_cart_flash);
        if (save_id != SAVE_ID_NONE)
            return save_id;
    }

    if (mcu_native_save_id_get(&save_id, target)) {
        mcu_native_finish();
        return save_id;
    }

    mcu_native_finish();
    return SAVE_ID_NONE;
}

bool launch_set_save_id(uint32_t v, uint16_t target) {
    uint8_t prev_cart_flash = inportb(WS_CART_BANK_FLASH_PORT);
    uint16_t prev_sram_bank = inportw(WS_CART_EXTBANK_RAM_PORT);
    outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
    *NILE_IPC_SAVE_ID = (target & SAVE_ID_FOR_FLASH) ? v : SAVE_ID_NONE;
    outportw(WS_CART_EXTBANK_RAM_PORT, prev_sram_bank);
    outportb(WS_CART_BANK_FLASH_PORT, prev_cart_flash);

    bool result = mcu_native_save_id_set(v, target);
    mcu_native_finish();
    return result;
}

static const uint8_t __far elisa_font_string[] = {'E', 'L', 'I', 'S', 'A'};
static const uint16_t __far rom_banks[] = {
    2, 4, 8, 16, 32, 48, 64, 96, 128, 256, 512, 1024
};
static const uint16_t __far sram_sizes[] = {
    0, 32, 32, 128, 256, 512, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
static const uint16_t __far eeprom_sizes[] = {
    0, 128, 2048, 0, 0, 1024, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
static const uint8_t __far eeprom_bits[] = {
    0, 6, 10, 0, 0, 10
};
static const uint8_t __far eeprom_emu_control[] = {
    0, NILE_EMU_EEPROM_128B, NILE_EMU_EEPROM_2KB, 0, 0, NILE_EMU_EEPROM_1KB
};
static const uint8_t __far eeprom_mcu_control[] = {
    0, 2, 6, 0, 0, 5
};

int16_t launch_get_rom_metadata_psram(launch_rom_metadata_t *meta) {
    // TODO: Implement Freya support
    uint32_t size = bootstub_data->prog.size;
    if (size < 16)
        return ERR_FILE_FORMAT_INVALID;

    uint32_t fpos = bootstub_data->prog.size - 16;
    uint16_t bank = fpos >> 16;
    uint16_t offset = fpos & 0xFFFF;

    uint16_t rom0_bank = ws_bank_rom0_save(bank);
    uint16_t rom1_bank = ws_bank_rom1_save(bank + 1);

    memcpy(&meta->footer, MK_FP(WS_ROM0_SEGMENT + (offset >> 4), offset & 0xF), 16);

    ws_bank_rom0_restore(rom0_bank);
    ws_bank_rom1_restore(rom1_bank);

    if (meta->footer.jump_command != 0xEA || meta->footer.jump_segment == 0x0000)
        return ERR_FILE_NOT_EXECUTABLE;

    meta->rom_banks = meta->footer.rom_size > 0x0B ? 0 : rom_banks[meta->footer.rom_size];
    meta->sram_size = sram_sizes[meta->footer.save_type & 0xF] * 1024L;
    meta->eeprom_size = eeprom_sizes[meta->footer.save_type >> 4];
    meta->flash_size = 0;
    meta->rom_type = ROM_TYPE_UNKNOWN;

    return FR_OK;
}

__attribute__((noinline))
static bool launch_is_rom_static_freya(FIL *f) {
    uint8_t buffer[512 - 16];
    uint16_t br;
    int16_t result;

    result = f_lseek(f, 0x7fe00);
    if (result != FR_OK) return false;

    result = f_read(f, buffer, sizeof(buffer), &br);
    if (result != FR_OK) return false;

    for (int i = 0; i < (sizeof(buffer) - 4); i++) {
        if (buffer[i] == 'E' && buffer[i+1] == 'x' && buffer[i+2] == 't' && buffer[i+3] == 'r' && buffer[i+4] == 'a') {
            return true;
        }
    }

    return false;
}

bool launch_is_battery_required(launch_rom_metadata_t *meta) {
    if (meta->eeprom_size)
        return true;
    if (meta->sram_size)
        return meta->rom_type != ROM_TYPE_FREYA;
    return false;
}

int16_t launch_get_rom_metadata(const char *path, launch_rom_metadata_t *meta) {
    uint8_t tmp[5];
    uint16_t br;

    FIL f;
    int16_t result = f_open(&f, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK)
        return result;

    meta->id = f.obj.sclust;

    uint32_t size = f_size(&f);
    if (size < 16) {
        f_close(&f);
        return ERR_FILE_FORMAT_INVALID;
    }

    result = f_lseek(&f, size - 16);
    if (result != FR_OK) {
        f_close(&f);
        return result;
    }

    result = f_read(&f, &(meta->footer), sizeof(rom_footer_t), &br);
    if (result != FR_OK) {
        f_close(&f);
        return result;
    }

    if (meta->footer.jump_command != 0xEA || meta->footer.jump_segment == 0x0000) {
        f_close(&f);
        return ERR_FILE_FORMAT_INVALID;
    }

    meta->rom_banks = meta->footer.rom_size > 0x0B ? 0 : rom_banks[meta->footer.rom_size];
    meta->sram_size = sram_sizes[meta->footer.save_type & 0xF] * 1024L;
    meta->eeprom_size = eeprom_sizes[meta->footer.save_type >> 4];
    meta->flash_size = 0;

    meta->rom_type = ROM_TYPE_UNKNOWN;
    if (meta->footer.publisher_id == 0x00
        && meta->footer.game_id == 0x00
        && meta->footer.save_type == 0x04
        && meta->footer.mapper == 0x01
        && size == 0x80000) {

        // Freya image test
        result = f_lseek(&f, 0x70000);
        if (result != FR_OK) {
            f_close(&f);
            return result;
        }

        result = f_read(&f, tmp, sizeof(elisa_font_string), &br);
        if (result != FR_OK) {
            f_close(&f);
            return result;
        }

        if (!_fmemcmp(elisa_font_string, tmp, sizeof(elisa_font_string))) {
            if (!launch_is_rom_static_freya(&f)) {
                meta->rom_type = ROM_TYPE_FREYA;
                meta->flash_size = 0x80000;
            }
        }
    } else {
        const char __far *ext = strrchr(path, '.');
        if (ext != NULL) {
            if (!strcasecmp(ext, s_file_ext_ws) || !strcasecmp(ext, s_file_ext_wsc)) {
                meta->rom_type = ROM_TYPE_WS;
            } else if (!strcasecmp(ext, s_file_ext_pc2)) {
                meta->rom_type = ROM_TYPE_PCV2;
            }
        }
    }

    f_close(&f);
    return FR_OK;
}

static int16_t preallocate_file(const char *path, FIL *fp, uint8_t fill_byte, uint32_t file_size, const char *src_path, uint16_t lk, uint16_t lk_overdump) {
    uint8_t stack_buffer[CONFIG_MEMLAYOUT_STACK_BUFFER_SIZE];
    uint8_t *buffer;
    uint16_t buffer_size;
    int16_t result, result2;
    uint16_t bw;
    ui_popup_dialog_config_t dlg = {0};

    if (sector_buffer_is_active()) {
        buffer = sector_buffer;
        buffer_size = sizeof(sector_buffer);
    } else {
        buffer = stack_buffer;
        buffer_size = sizeof(stack_buffer);
    }

    result = f_open(fp, path, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
    if (result != FR_OK)
        return result;

    // Do not overwrite the file if it already has the right size.
    if (f_size(fp) >= file_size) {
        if (f_size(fp) > file_size) {
            dlg.title = lang_keys[lk_overdump];
            dlg.description = lang_keys[LK_DIALOG_SAVE_OVERDUMP_DESCRIPTION];
            dlg.buttons[0] = LK_YES;
            dlg.buttons[1] = LK_NO;

            ui_popup_dialog_draw(&dlg);
            if (ui_popup_dialog_action(&dlg, 1) == 1) {
                result = ERR_USER_EXIT_REQUESTED;
                goto preallocate_file_end;
            }
            ui_popup_dialog_clear(&dlg);
        }
        goto preallocate_file_end_no_dialog;
    }

    dlg.title = lang_keys[lk];
    dlg.progress_max = (file_size + buffer_size - 1) / buffer_size;
    ui_popup_dialog_draw(&dlg);

    // Try to ensure a contiguous area for the file.
    result = f_expand(fp, file_size, 0);

    // Write the remaining data.
    if (src_path != NULL) {
        // Copy from src_path to path.
        FIL src_fp;

        result = f_open(&src_fp, src_path, FA_READ | FA_OPEN_EXISTING);
        if (result != FR_OK)
            return result;

        if (f_size(&src_fp) != file_size) {
            result = FR_INT_ERR;
            goto preallocate_file_copy_end;
        }

        for (uint32_t i = 0; i < file_size; i += buffer_size) {
            uint16_t to_write = buffer_size;
            if ((file_size - i) < to_write)
                to_write = file_size - i;
            result = f_read(&src_fp, buffer, to_write, &bw);
            if (result != FR_OK)
                goto preallocate_file_copy_end;
            result = f_write(fp, buffer, to_write, &bw);
            if (result != FR_OK)
                goto preallocate_file_copy_end;

            dlg.progress_step++;
            ui_popup_dialog_draw_update(&dlg);
        }
preallocate_file_copy_end:
        f_close(&src_fp);
    } else {
        // Fill bytes.
        memset(buffer, fill_byte, buffer_size);
        result = f_lseek(fp, f_size(fp));
        if (result != FR_OK)
            goto preallocate_file_end;

        for (uint32_t i = f_tell(fp); i < file_size; i += buffer_size) {
            uint16_t to_write = buffer_size;
            if ((file_size - i) < to_write)
                to_write = file_size - i;
            result = f_write(fp, buffer, to_write, &bw);
            if (result != FR_OK)
                goto preallocate_file_end;

            dlg.progress_step++;
            ui_popup_dialog_draw_update(&dlg);
        }
    }

preallocate_file_end:
    ui_popup_dialog_clear(&dlg);

preallocate_file_end_no_dialog:
    result2 = f_lseek(fp, 0);
    if (result == FR_OK && result2 != FR_OK)
        return result2;
    return result;
}

#define MAX_WRITE_EEPROM_WORDS 64

static int16_t launch_write_eeprom(FIL *fp, uint8_t *buffer, uint16_t words) {
    int16_t result = FR_OK;

    for (uint16_t i = 0; i < words; i += MAX_WRITE_EEPROM_WORDS) {
        int to_read = words > MAX_WRITE_EEPROM_WORDS ? MAX_WRITE_EEPROM_WORDS : words;
        nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
        if (!mcu_native_eeprom_read_data(buffer, i, to_read)) {
            result = ERR_MCU_COMM_FAILED;
            break;
        }

        nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_TF);
        result = f_write(fp, buffer, to_read << 1, NULL);
        if (result != FR_OK)
            break;
    }

    return result;
}

static int16_t launch_read_eeprom(FIL *fp, uint8_t mode, uint16_t words) {
    uint16_t w;
    uint16_t br;
    int16_t result;

    ws_eeprom_handle_t h = ws_eeprom_handle_cartridge(eeprom_bits[mode]);
    outportb(IO_NILE_EMU_CNT, eeprom_emu_control[mode]);

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
    ws_eeprom_write_unlock(h);
    for (uint16_t i = 0; i < words; i++) {
         nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_TF);
         result = f_read(fp, &w, 2, &br);
         if (result != FR_OK)
             break;
         nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
         if (!ws_eeprom_write_word(h, i << 1, w)) {
             result = ERR_MCU_COMM_FAILED;
             break;
         }
    }
    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
    ws_eeprom_write_lock(h);

    return result;
}

static void launch_backup_progress_update(ui_popup_dialog_config_t *dlg, uint32_t step, uint32_t max) {
    dlg->progress_step = step >> 7;
    dlg->progress_max = max >> 7;
    ui_popup_dialog_draw_update(dlg);
}

int16_t launch_backup_save_data(void) {
    FIL fp, save_fp;
    char buffer[FF_LFN_BUF + 33];
    char *key, *value;
    ini_next_result_t ini_result;
    int16_t result;
    ui_popup_dialog_config_t dlg = {0};
    uint32_t id = 0;
    uint16_t value_num;

    strcpy(buffer, s_path_save_ini);
    result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
    // If the .ini file doesn't exist, skip.
    if (result == FR_NO_FILE)
        return FR_OK;
    if (result != FR_OK)
        return result;

    dlg.title = lang_keys[LK_DIALOG_STORE_SAVE];
    dlg.progress_max = 1;
    ui_popup_dialog_draw(&dlg);
    ui_show();

    while (true) {
        ini_result = ini_next(&fp, buffer, sizeof(buffer), &key, &value);
        if (ini_result == INI_NEXT_ERROR) {
            result = FR_INT_ERR;
            goto launch_backup_save_data_return_result;
        } else if (ini_result == INI_NEXT_FINISHED) {
            break;
        } else if (ini_result == INI_NEXT_CATEGORY) {
            // TODO: Pay attention to this.
        } else if (ini_result == INI_NEXT_KEY_VALUE) {
            if (!strcasecmp(key, s_save_ini_id)) {
                id = atol(value);
                continue;
            }

            if (!strcasecmp(key, s_save_ini_freya_ram0)) {
                uint32_t fetched_id = launch_get_save_id(SAVE_ID_FOR_SRAM);
                if (id != fetched_id) {
                    result = ERR_SAVE_CORRUPT;
                    goto launch_backup_save_data_return_result;
                }

                result = launch_athena_restore_ram0(value);
                if (result != FR_OK) {
                    goto launch_backup_save_data_return_result;
                }
                continue;
            }

            uint16_t file_type = 0;
            if (!strcasecmp(key, s_save_ini_sram)) file_type = SAVE_ID_FOR_SRAM;
            else if (!strcasecmp(key, s_save_ini_eeprom)) file_type = SAVE_ID_FOR_EEPROM;
            else if (!strcasecmp(key, s_save_ini_flash)) file_type = SAVE_ID_FOR_FLASH;
            if (file_type) {
                key = (char*) strchr(value, '|');
                if (key == NULL) continue;
                key[0] = 0;
                value_num = atoi(value);
                value = key + 1;

                uint32_t fetched_id = launch_get_save_id(file_type);
                if (id != fetched_id) {
                    result = file_type == SAVE_ID_FOR_FLASH ? ERR_SAVE_PSRAM_CORRUPT : ERR_SAVE_CORRUPT;
                    goto launch_backup_save_data_return_result;
                }

                result = f_open(&save_fp, value, FA_OPEN_EXISTING | FA_WRITE);
                if (result != FR_OK) {
                    // TODO: Handle FR_NO_FILE by preallocating a new file?
                    goto launch_backup_save_data_return_result;
                }

                ui_popup_dialog_clear_progress(&dlg);
                if (file_type == SAVE_ID_FOR_SRAM) {
                    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
                    result = f_write_sram_banked(&save_fp, 0, f_size(&save_fp), (fbanked_progress_callback_t) launch_backup_progress_update, &dlg);
                } else if (file_type == SAVE_ID_FOR_EEPROM) {
                    result = launch_write_eeprom(&save_fp, buffer, value_num >> 1);
                } else if (file_type == SAVE_ID_FOR_FLASH) {
                    uint16_t prev_bank = inportw(WS_CART_EXTBANK_ROM0_PORT);
                    outportw(WS_CART_EXTBANK_ROM0_PORT, (f_size(&save_fp) - 1) >> 16);
                    memcpy(buffer, MK_FP(WS_ROM0_SEGMENT, 0xFFF0), 16);
                    outportw(WS_CART_EXTBANK_ROM0_PORT, prev_bank);

                    // Only restore flash if contents appear bootable.
                    // FIXME: This skips the ROM footer to avoid losing the original entrypoint
                    // for runtime patches, which is not ideal.
                    if (((uint8_t) buffer[0]) == 0xEA && ((uint8_t) buffer[4]) >= 0x10) {
                        result = f_write_rom_banked(&save_fp, 0, f_size(&save_fp) - 16, (fbanked_progress_callback_t) launch_backup_progress_update, &dlg);
                    } else {
                        result = ERR_SAVE_PSRAM_CORRUPT;
                    }
                }

                f_close(&save_fp);
                if (result != FR_OK) {
                    goto launch_backup_save_data_return_result;
                }
            }
        }
    }

launch_backup_save_data_return_result:
    f_close(&fp);
    strcpy(buffer, s_path_save_ini);
    f_unlink(buffer);
    // Clear save ID
    launch_set_save_id(SAVE_ID_NONE, 0);

    ui_popup_dialog_clear(&dlg);
    return result;
}

int16_t launch_restore_save_data(char *path, const launch_rom_metadata_t *meta) {
    char dst_cwd[FF_LFN_BUF + 4];
    char dst_path[FF_LFN_BUF + 4];
    char tmp_buf[20];
    FIL fp;
    int16_t result;

    uint16_t save_target = 0;
    if (meta->sram_size)   save_target |= SAVE_ID_FOR_SRAM;
    if (meta->eeprom_size) save_target |= SAVE_ID_FOR_EEPROM;
    if (meta->flash_size)  save_target |= SAVE_ID_FOR_FLASH;

    // write save ID to MCU
    // (error only if save ID was required)
    if (!launch_set_save_id(meta->id, save_target) && save_target) {
        result = ERR_MCU_COMM_FAILED;
        goto launch_restore_save_data_return_result;
    }

    // extension-editable version of "path"
    strcpy(dst_path, path);
    char *ext_loc = (char*) strrchr(dst_path, '.');
    if (ext_loc == NULL)
        ext_loc = dst_path + strlen(dst_path);

    // restore or create data
    if (meta->sram_size != 0) {
        strcpy(ext_loc, s_file_ext_sram);
        result = preallocate_file(dst_path, &fp, 0xFF, meta->sram_size, NULL, LK_DIALOG_PREPARE_SAVE, LK_DIALOG_SAVE_OVERDUMP_TITLE_SRAM);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        // copy data to SRAM
        outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
        result = f_read_sram_banked(&fp, 0, f_size(&fp), NULL, NULL);
        if (result != FR_OK) {
            f_close(&fp);
            goto launch_restore_save_data_return_result;
        }

        result = f_close(&fp);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;
    }
    if (meta->eeprom_size != 0) {
        strcpy(ext_loc, s_file_ext_eeprom);
        result = preallocate_file(dst_path, &fp, 0xFF, meta->eeprom_size, NULL, LK_DIALOG_PREPARE_SAVE, LK_DIALOG_SAVE_OVERDUMP_TITLE_EEPROM);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        // initialize MCU
        if (!mcu_native_eeprom_set_type(eeprom_mcu_control[meta->footer.save_type >> 4])) {
            result = ERR_MCU_COMM_FAILED;
            goto launch_restore_save_data_return_result;
        }
	    mcu_native_finish();

        // switch MCU to EEPROM mode
        if (!mcu_native_set_mode(1)) {
            result = ERR_MCU_COMM_FAILED;
            goto launch_restore_save_data_return_result;
        }

        // copy data to EEPROM
        result = launch_read_eeprom(&fp, meta->footer.save_type >> 4,
            f_size(&fp) >> 1);
        if (result != FR_OK) {
            f_close(&fp);
            goto launch_restore_save_data_return_result;
        }

        mcu_native_finish();

        result = f_close(&fp);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;
    }
    if (meta->flash_size != 0) {
        strcpy(ext_loc, s_file_ext_flash);
        result = preallocate_file(dst_path, &fp, 0xFF, meta->flash_size, path, LK_DIALOG_PREPARE_FLASH, LK_DIALOG_SAVE_OVERDUMP_TITLE_FLASH);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        result = f_close(&fp);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        // Use .flash instead of .ws/.wsc to boot on this platform.
        strcpy(path + (ext_loc - dst_path), s_file_ext_flash);
    }

    result = f_getcwd(dst_cwd, sizeof(dst_cwd) - 1);
    if (result != FR_OK)
        goto launch_restore_save_data_ini_end;

    char *dst_cwd_end = dst_cwd + strlen(dst_cwd) - 1;
    if (*dst_cwd_end != '/') {
        *(++dst_cwd_end) = '/';
        *(++dst_cwd_end) = 0;
    }

    strcpy(tmp_buf, s_path_save_ini);

    if (!save_target) {
        // remove .INI file, if it exists
        result = f_unlink(tmp_buf);
        if (result != FR_OK && result != FR_NO_FILE)
            return result;
        result = FR_OK;
        goto launch_restore_save_data_return_result;
    }

    // generate .INI file
    result = f_open(&fp, tmp_buf, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK)
        goto launch_restore_save_data_return_result;

    result = f_puts(s_save_ini_start, &fp) < 0 ? FR_INT_ERR : FR_OK;
    if (result != FR_OK)
        goto launch_restore_save_data_ini_end;

    // ID has to be parsed before other entries
    result = f_printf(&fp, s_save_ini_id_entry, meta->id) < 0 ? FR_INT_ERR : FR_OK;
    if (result != FR_OK)
        goto launch_restore_save_data_ini_end;

    // Write FLASH entry first, as it has a weaker save ID condition
    // (present in IPC, as opposed to present on MCU)
    if (meta->flash_size != 0) {
        strcpy(ext_loc, s_file_ext_flash);
        result = f_printf(&fp, s_save_ini_entry,
            (const char __far*) s_save_ini_flash,
            (uint32_t) meta->flash_size,
            (const char __far*) dst_cwd,
            (const char __far*) dst_path) < 0 ? FR_INT_ERR : FR_OK;
        if (result != FR_OK)
            goto launch_restore_save_data_ini_end;
    }

    if (meta->sram_size != 0) {
        strcpy(ext_loc, s_file_ext_sram);
        result = f_printf(&fp, s_save_ini_entry,
            (const char __far*) s_save_ini_sram,
            (uint32_t) meta->sram_size,
            (const char __far*) dst_cwd,
            (const char __far*) dst_path) < 0 ? FR_INT_ERR : FR_OK;
        if (result != FR_OK)
            goto launch_restore_save_data_ini_end;
    }

    if (meta->eeprom_size != 0) {
        strcpy(ext_loc, s_file_ext_eeprom);
        result = f_printf(&fp, s_save_ini_entry,
            (const char __far*) s_save_ini_eeprom,
            (uint32_t) meta->eeprom_size,
            (const char __far*) dst_cwd,
            (const char __far*) dst_path) < 0 ? FR_INT_ERR : FR_OK;
        if (result != FR_OK)
            goto launch_restore_save_data_ini_end;
    }

launch_restore_save_data_ini_end:
    result = result || f_close(&fp);
launch_restore_save_data_return_result:
    return result;
}

bool launch_ui_handle_battery_missing_error(launch_rom_metadata_t *meta) {
    ui_popup_dialog_config_t dlg = {0};

    dlg.title = lang_keys[LK_PROMPT_NO_BATTERY_TITLE];
    dlg.description = lang_keys[LK_PROMPT_NO_BATTERY];
    dlg.buttons[0] = LK_YES;
    dlg.buttons[1] = LK_NO;

    ui_popup_dialog_draw(&dlg);
    ui_show();

    if (ui_popup_dialog_action(&dlg, 1) == 0) {
        return true;
    } else {
        return false;
    }
}

bool launch_ui_handle_mcu_comm_error(launch_rom_metadata_t *meta) {
    ui_popup_dialog_config_t dlg = {0};

    dlg.title = lang_keys[LK_ERROR_MCU_COMM_FAILED];
    dlg.description = lang_keys[LK_PROMPT_FUNCTIONALITY_UNAVAILABLE];
    dlg.buttons[0] = LK_YES;
    dlg.buttons[1] = LK_NO;

    ui_popup_dialog_draw(&dlg);
    ui_show();

    if (ui_popup_dialog_action(&dlg, 1) == 0) {
        // Adjust metadata to disable all MCU-reliant features
        meta->eeprom_size = 0;   // EEPROM
        meta->footer.mapper = 0; // RTC
        return true;
    } else {
        return false;
    }
}

int16_t launch_set_bootstub_file_entry(const char *path, bootstub_file_entry_t *entry) {
    FILINFO fp;
    int16_t result = f_stat(path, &fp);
    if (result != FR_OK) {
        return result;
    }
    entry->cluster = fp.fclust;
    entry->size = fp.fsize;
    return FR_OK;
}

int16_t launch_rom_via_bootstub(const launch_rom_metadata_t *meta) {
    extern const void __bank_bootstub;
    extern const void __bank_gfx_bootstub_tiles;

    if (bootstub_data->prog.size > 16*1024*1024L) {
        return ERR_FILE_TOO_LARGE;
    }

    outportw(WS_DISPLAY_CTRL_PORT, 0);

    // Disable IRQs - avoid other code interfering/overwriting memory
    ia16_disable_irq();

    // Populate bootstub data
    bootstub_data->data_base = fs.database;
    bootstub_data->cluster_table_base = fs.fatbase;
    bootstub_data->cluster_size = fs.csize;
    bootstub_data->fat_entry_count = fs.n_fatent;
    bootstub_data->fs_type = fs.fs_type;

    if (meta != NULL) {
        bootstub_data->rom_banks = meta->rom_banks;
        bootstub_data->prog_sram_mask = (meta->sram_size - 1) >> 16;
        bootstub_data->prog_emu_cnt =
              (meta->eeprom_size ? eeprom_emu_control[meta->footer.save_type >> 4] : 0)
            | (meta->flash_size ? NILE_EMU_FLASH_FSM : 0)
            | ((meta->footer.flags & 0x04) ? NILE_EMU_ROM_BUS_16BIT : NILE_EMU_ROM_BUS_8BIT);
        // TODO: Emulate EEPROM N/C (remove meta->eeprom_size check)
        bootstub_data->prog_pow_cnt =
              (meta->sram_size ? NILE_POW_SRAM : 0)
            | ((meta->footer.mapper != 1 && meta->eeprom_size) ? NILE_POW_IO_2001 : 0)
            | (meta->footer.mapper != 0 ? NILE_POW_IO_2003 : 0);
        bootstub_data->prog_flags = meta->footer.flags;
        bootstub_data->prog_rom_type = meta->rom_type;
    } else {
        bootstub_data->rom_banks = 0;
        bootstub_data->prog_sram_mask = 7;
        bootstub_data->prog_emu_cnt = 0;
        bootstub_data->prog_pow_cnt = inportb(IO_NILE_POW_CNT);
        bootstub_data->prog_flags = 0x04;
        bootstub_data->prog_rom_type = ROM_TYPE_UNKNOWN;
    }
    bootstub_data->prog_flags2 = (settings.prog.flags & SETTING_PROG_FLAG_SRAM_OVERCLOCK) ? 0x00 : 0x0A;
    bootstub_data->prog_patches = bootstub_data->prog_rom_type == ROM_TYPE_FREYA ? BOOTSTUB_PROG_PATCH_FREYA_SOFT_RESET : 0;
    if (bootstub_data->prog_rom_type == ROM_TYPE_PCV2) {
        bootstub_data->start_pointer = MK_FP(0x4000, 0x0010);
    } else {
        bootstub_data->start_pointer = MK_FP(0xFFFF, 0x0000);
    }

    // Lock IEEPROM
    if (meta != NULL && !(meta->footer.game_version & 0x80)) {
        outportw(WS_IEEP_CTRL_PORT, WS_IEEP_CTRL_PROTECT);
    }

    // Switch MCU to RTC mode
    if (meta == NULL || !meta->eeprom_size) {
        if (meta != NULL && meta->footer.mapper == 1) {
            outportb(IO_NILE_IRQ_ENABLE, NILE_IRQ_MCU);
            mcu_native_set_mode(2);
        } else {
            outportb(IO_NILE_IRQ_ENABLE, 0);
            mcu_native_set_mode(0xFF);
        }
    }

    ws_int_ack_all();
    mcu_native_finish();

    // Initialize bootstub data
    if (ws_system_is_color_active()) {
        ws_system_set_mode(WS_MODE_COLOR);
        outportw(WS_CART_EXTBANK_ROM0_PORT, (uint8_t) &__bank_gfx_bootstub_tiles);
        ws_gdma_copy((void*) 0x3200, gfx_bootstub_tiles, gfx_bootstub_tiles_size);
        outportw(WS_CART_EXTBANK_ROM0_PORT, (uint8_t) &__bank_bootstub);
        ws_gdma_copy((void*) 0x00c0, bootstub, bootstub_size);
    } else {
        ws_system_set_mode(WS_MODE_COLOR);
        outportw(WS_CART_EXTBANK_ROM0_PORT, (uint8_t) &__bank_gfx_bootstub_tiles);
        memcpy((void*) 0x3200, gfx_bootstub_tiles, gfx_bootstub_tiles_size);
        outportw(WS_CART_EXTBANK_ROM0_PORT, (uint8_t) &__bank_bootstub);
        memcpy((void*) 0x00c0, bootstub, bootstub_size);
    }
    // Jump to bootstub
    asm volatile("ljmp $0x0000,$0x00c0\n");
    return true;
}

int16_t launch_in_psram(uint32_t size) {
    int16_t result;
    launch_rom_metadata_t meta;

    bootstub_data->prog.size = size;

    // Try reading as ROM
    result = launch_get_rom_metadata_psram(&meta);
    if (result == FR_OK) {
        bootstub_data->prog.cluster = BOOTSTUB_CLUSTER_AT_PSRAM;
        result = launch_rom_via_bootstub(&meta);
    }

    // Try reading as BFB
    if (bootstub_data->prog.size <= 65536) {
        launch_bfb_in_psram();
    }

    return FR_INT_ERR;
}
