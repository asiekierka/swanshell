/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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
#include "mcu.h"
#include "strings.h"
#include "../../build/menu/build/bootstub_bin.h"
#include "../../build/menu/assets/menu/bootstub_tiles.h"
#include "lang.h"
#include "ui/ui.h"
#include "ui/ui_popup_dialog.h"
#include "util/file.h"
#include "util/ini.h"

__attribute__((section(".iramx_0040")))
uint16_t ipl0_initial_regs[16];
__attribute__((section(".iramCx_c000")))
uint8_t sector_buffer[2048];

extern FATFS fs;

#define NILE_IPC_SAVE_ID ((volatile uint32_t __far*) MK_FP(0x1000, 512 - sizeof(uint32_t)))

/*
void ui_boot(const char *path) {
    FIL fp;
    
	uint8_t result = f_open(&fp, path, FA_READ);
	if (result != FR_OK) {
        // TODO
        return;
	}

    outportw(IO_DISPLAY_CTRL, 0);
	outportb(IO_CART_FLASH, CART_FLASH_ENABLE);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return;
    }
    if (size < 65536L) {
        size = 65536L;
    }
    uint32_t real_size = round2(size);
    uint16_t offset = (real_size - size);
    uint16_t bank = (real_size - size) >> 16;
    uint16_t total_banks = real_size >> 16;

	while (bank < total_banks) {
		outportw(IO_BANK_2003_RAM, bank);
		if (offset < 0x8000) {
			if ((result = f_read(&fp, MK_FP(0x1000, offset), 0x8000 - offset, NULL)) != FR_OK) {
                ui_init();
				return;
			}
			offset = 0x8000;
		}
		if ((result = f_read(&fp, MK_FP(0x1000, offset), -offset, NULL)) != FR_OK) {
            ui_init();
			return;
		}
		offset = 0x0000;
		bank++;
	}
    
    clear_registers(true);
    launch_ram_asm(MK_FP(0xFFFF, 0x0000), );
} */

uint32_t launch_get_ipc_save_id(void) {
    uint16_t prev_sram_bank = inportw(IO_BANK_2003_RAM);
    outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
    uint32_t save_id = *NILE_IPC_SAVE_ID;
    outportw(IO_BANK_2003_RAM, prev_sram_bank);
    return save_id;
}

void launch_set_ipc_save_id(uint32_t v) {
    uint16_t prev_sram_bank = inportw(IO_BANK_2003_RAM);
    outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
    *NILE_IPC_SAVE_ID = v;
    outportw(IO_BANK_2003_RAM, prev_sram_bank);
}

static const uint8_t __far elisa_font_string[] = {'E', 'L', 'I', 'S', 'A'};
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
typedef enum {
    SAVE_TYPE_NONE = 0,
    SAVE_TYPE_SRAM,
    SAVE_TYPE_EEPROM,
    SAVE_TYPE_FLASH
} launch_save_type_t;

int16_t launch_get_rom_metadata(const char *path, launch_rom_metadata_t *meta) {
    uint8_t tmp[5];
    uint16_t br;
    bool elisa_found = false;

    FIL f;
    int16_t result = f_open(&f, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK)
        return result;

    meta->id = f.obj.sclust;

    uint32_t size = f_size(&f);
    if (size == 0x80000) {
        result = f_lseek(&f, 0x70000);
        if (result != FR_OK)
            return result;

        result = f_read(&f, tmp, sizeof(elisa_font_string), &br);
        if (result != FR_OK)
            return result;

        elisa_found = !_fmemcmp(elisa_font_string, tmp, sizeof(elisa_font_string));
    }

    result = f_lseek(&f, size - 16);
    if (result != FR_OK)
        return result;

    result = f_read(&f, &(meta->footer), sizeof(rom_footer_t), &br);
    if (result != FR_OK)
        return result;

    meta->sram_size = sram_sizes[meta->footer.save_type & 0xF] * 1024L;
    meta->eeprom_size = eeprom_sizes[meta->footer.save_type >> 4];
    meta->flash_size = 0;
    if (elisa_found
        && meta->footer.publisher_id == 0x00
        && meta->footer.game_id == 0x00
        && meta->footer.save_type == 0x04
        && meta->footer.mapper == 0x01) {

        meta->flash_size = 0x80000;
    }

    return FR_OK;
}

static int16_t preallocate_file(const char *path, FIL *fp, uint8_t fill_byte, uint32_t file_size, const char *src_path) {
    uint8_t stack_buffer[16];
    uint8_t *buffer;
    uint16_t buffer_size;
    int16_t result, result2;
    uint16_t bw;

    if (ws_system_color_active()) {
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
    if (f_size(fp) >= file_size)
        goto preallocate_file_end;

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
        }
    }

preallocate_file_end:
    result2 = f_lseek(fp, 0);
    if (result == FR_OK && result2 != FR_OK)
        return result2;
    return result;
}

#define MAX_WRITE_EEPROM_WORDS 64

static int16_t launch_write_eeprom(FIL *fp, uint8_t *buffer, uint16_t words) {
    int16_t result;

    for (uint16_t i = 0; i < words; i += MAX_WRITE_EEPROM_WORDS) {
        int to_read = words > MAX_WRITE_EEPROM_WORDS ? MAX_WRITE_EEPROM_WORDS : words;
        nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
        if (!mcu_native_eeprom_read_data(buffer, i, to_read)) {
            result = ERR_EEPROM_COMM_FAILED;
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
    int16_t result;

    ws_eeprom_handle_t h = ws_eeprom_handle_cartridge(eeprom_bits[mode]);
    outportb(IO_NILE_EMU_CNT, eeprom_emu_control[mode]);

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
    ws_eeprom_write_unlock(h);
    for (uint16_t i = 0; i < words; i++) {
         nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_TF);
         result = f_read(fp, &w, 2, NULL);
         if (result != FR_OK)
             break;
         nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
         if (!ws_eeprom_write_word(h, i << 1, w)) {
             result = ERR_EEPROM_COMM_FAILED;
             break;
         }
    }
    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_NONE);
    ws_eeprom_write_lock(h);

    return result;
}

DEFINE_STRING_LOCAL(s_save_ini_start, "[Save]\n");
DEFINE_STRING_LOCAL(s_save_ini_id, "ID");
DEFINE_STRING_LOCAL(s_save_ini_sram, "SRAM");
DEFINE_STRING_LOCAL(s_save_ini_eeprom, "EEPROM");
DEFINE_STRING_LOCAL(s_save_ini_flash, "Flash");
DEFINE_STRING_LOCAL(s_save_ini_entry, "%s=%ld|%s%s\n");
DEFINE_STRING_LOCAL(s_save_ini_id_entry, "ID=%ld\n");

int16_t launch_backup_save_data(void) {
    FIL fp, save_fp;
    char buffer[FF_LFN_BUF + 4];
    char *key, *value;
    ini_next_result_t ini_result;
    int16_t result;
    ui_popup_dialog_config_t dlg = {0};
    uint32_t id = 0;
    uint32_t mcu_id;
    uint16_t value_num;

    strcpy(buffer, s_path_save_ini);
    result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
    // If the .ini file doesn't exist, skip.
    if (result == FR_NO_FILE)
        return FR_OK;
    if (result != FR_OK)
        return result;

    dlg.title = lang_keys[LK_DIALOG_STORE_SAVE];
    ui_popup_dialog_draw(&dlg);
    ui_show();

    // read save ID from MCU
    if (!mcu_native_save_id_get(&mcu_id)) {
        result = ERR_MCU_COMM_FAILED;
        goto launch_backup_save_data_return_result;
    }
    mcu_native_finish();

    while (true) {
        ini_result = ini_next(&fp, buffer, sizeof(buffer), &key, &value);
        if (ini_result == INI_NEXT_ERROR) {
            result = FR_INT_ERR;
            f_close(&fp);
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

            uint8_t file_type = SAVE_TYPE_NONE;
            if (!strcasecmp(key, s_save_ini_sram)) file_type = SAVE_TYPE_SRAM;
            else if (!strcasecmp(key, s_save_ini_eeprom)) file_type = SAVE_TYPE_EEPROM;
            else if (!strcasecmp(key, s_save_ini_flash)) file_type = SAVE_TYPE_FLASH;
            if (file_type != SAVE_TYPE_NONE) {
                key = (char*) strchr(value, '|');
                if (key != NULL) {
                    key[0] = 0;
                    value_num = atoi(value);
                    value = key + 1;
                }

                if (file_type == SAVE_TYPE_FLASH) {
                    if (id != launch_get_ipc_save_id()) {
                        result = ERR_SAVE_CORRUPT;
                        goto launch_backup_save_data_return_result;    
                    }
                } else {
                    if (id != mcu_id) {
                        result = ERR_SAVE_CORRUPT;
                        goto launch_backup_save_data_return_result;
                    }
                }

                result = f_open(&save_fp, value, FA_OPEN_EXISTING | FA_WRITE);
                if (result != FR_OK) {
                    // TODO: Handle FR_NO_FILE by preallocating a new file.
                    f_close(&fp);
                    goto launch_backup_save_data_return_result;
                }

                if (file_type == SAVE_TYPE_SRAM) {
                    outportb(IO_CART_FLASH, 0);
                    result = f_write_sram_banked(&save_fp, 0, f_size(&save_fp), NULL);
                } else if (file_type == SAVE_TYPE_EEPROM) {
                    result = launch_write_eeprom(&save_fp, buffer, value_num >> 1);
                } else if (file_type == SAVE_TYPE_FLASH) {
                    uint16_t prev_bank = inportw(IO_BANK_2003_ROM0);
                    outportw(IO_BANK_2003_ROM0, (f_size(&save_fp) - 1) >> 16);
                    memcpy(buffer, MK_FP(0x1000, 0xFFF0), 16);
                    outportw(IO_BANK_2003_ROM0, prev_bank);
                    
                    // Only restore flash if contents appear bootable
                    if (((uint8_t) buffer[0]) == 0xEA && ((uint8_t) buffer[4]) > 0x10) {
                        result = f_write_rom_banked(&save_fp, 0, f_size(&save_fp), NULL);
                    } else {
                        result = ERR_SAVE_CORRUPT;
                    }
                }

                f_close(&save_fp);
                if (result != FR_OK) {
                    f_close(&fp);
                    goto launch_backup_save_data_return_result;
                }
            }
        }
    }

    f_close(&fp);
    strcpy(buffer, s_path_save_ini);
    f_unlink(buffer);
launch_backup_save_data_return_result:
    ui_popup_dialog_clear(&dlg);
    return result;
}

int16_t launch_restore_save_data(char *path, const launch_rom_metadata_t *meta) {
    char dst_cwd[FF_LFN_BUF + 4];
    char dst_path[FF_LFN_BUF + 4];
    char tmp_buf[20];
    FIL fp;
    int16_t result;
    ui_popup_dialog_config_t dlg = {0};

    bool has_save_data = meta->sram_size || meta->eeprom_size || meta->flash_size;

    if (has_save_data) {
        dlg.title = lang_keys[LK_DIALOG_PREPARE_SAVE];
        ui_popup_dialog_draw(&dlg);
    }

    // write save ID to MCU
    if (!mcu_native_save_id_set(has_save_data ? meta->id : 0)) {
        result = ERR_MCU_COMM_FAILED;
        goto launch_restore_save_data_return_result;
    }
    mcu_native_finish();

    // extension-editable version of "path"
    strcpy(dst_path, path);
    char *ext_loc = (char*) strrchr(dst_path, '.');
    if (ext_loc == NULL)
        ext_loc = dst_path + strlen(dst_path);

    // restore or create data
    if (meta->sram_size != 0) {
        strcpy(ext_loc, s_file_ext_sram);
        result = preallocate_file(dst_path, &fp, 0xFF, meta->sram_size, NULL);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        // copy data to SRAM
        outportb(IO_CART_FLASH, 0);
        result = f_read_sram_banked(&fp, 0, f_size(&fp), NULL);
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
        result = preallocate_file(dst_path, &fp, 0xFF, meta->eeprom_size, NULL);
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
        result = preallocate_file(dst_path, &fp, 0xFF, meta->flash_size, path);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        result = f_close(&fp);
        if (result != FR_OK)
            goto launch_restore_save_data_return_result;

        // Use .flash instead of .ws/.wsc to boot on this platform.
        strcpy(path + (ext_loc - dst_path), s_file_ext_flash);

        launch_set_ipc_save_id(meta->id);
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

    if (!has_save_data) {
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
    ui_popup_dialog_clear(&dlg);
    return result;
}

int16_t launch_rom_via_bootstub(const char *path, const launch_rom_metadata_t *meta) {
    FILINFO fp;
    
	int16_t result = f_stat(path, &fp);
	if (result != FR_OK) {
        return result;
	}

    outportw(IO_DISPLAY_CTRL, 0);

    // Disable IRQs - avoid other code interfering/overwriting memory
    cpu_irq_disable();

    // Initialize bootstub graphics
    if (ws_system_color_active()) {
        ws_system_mode_set(WS_MODE_COLOR);
    }
    wsx_zx0_decompress((void*) 0x3200, gfx_bootstub_tiles);

    // Populate bootstub data
    bootstub_data->data_base = fs.database;
    bootstub_data->cluster_table_base = fs.fatbase;
    bootstub_data->cluster_size = fs.csize;
    bootstub_data->fat_entry_count = fs.n_fatent;
    bootstub_data->fs_type = fs.fs_type;
    
    bootstub_data->prog_cluster = fp.fclust;
    bootstub_data->prog_size = fp.fsize;
    if (meta != NULL) {
        bootstub_data->prog_sram_mask = (meta->sram_size - 1) >> 16;
        bootstub_data->prog_emu_cnt =
            meta->eeprom_size ? eeprom_emu_control[meta->footer.save_type >> 4] : 0;
        // TODO: Emulate EEPROM N/C (remove meta->eeprom_size check)
        bootstub_data->prog_pow_cnt =
              (meta->sram_size ? NILE_POW_SRAM : 0)
            | ((meta->footer.mapper != 1 && meta->eeprom_size) ? NILE_POW_IO_2001 : 0)
            | (meta->footer.mapper != 0 ? NILE_POW_IO_2003 : 0);
        // NOTE: 8-bit ROM bus width is currently not supported.
        bootstub_data->prog_flags = meta->footer.flags | 0x04;
    } else {
        bootstub_data->prog_sram_mask = 7;
        bootstub_data->prog_emu_cnt = 0;
        bootstub_data->prog_pow_cnt = inportb(IO_NILE_POW_CNT);
        bootstub_data->prog_flags = 0x04;
    }

    // Switch MCU to RTC mode
    if (!meta->eeprom_size) {
        mcu_native_set_mode(2);
        mcu_native_finish();
    }

    // Jump to bootstub
    memcpy((void*) 0x00c0, bootstub, bootstub_size);
    asm volatile("ljmp $0x0000,$0x00c0\n");
    return true;
}
