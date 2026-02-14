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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <nilefs.h>
#include <ws/system.h>
#include "errors.h"
#include "lang.h"
#include "../ui/ui.h"
#include "../util/file.h"
#include "../util/input.h"
#include "../util/memops.h"
#include "../util/util.h"
#include "settings.h"
#include "ui/bitmap.h"
#include "vgm/vgm.h"

static vgm_state_t *vgm_state;

void  __attribute__((interrupt, assume_ss_data)) vgm_interrupt_handler(void) {
    while (true) {
        outportw(WS_TIMER_HBL_RELOAD_PORT, 65535);
        uint16_t result = vgm_play(vgm_state);
        uint16_t ticks_elapsed = inportw(WS_TIMER_HBL_COUNTER_PORT) ^ 65535;
        if (result == VGM_PLAYBACK_FINISHED) {
            vgm_state->bank = 0xFF;
            outportb(WS_TIMER_CTRL_PORT, 0);
        } else if (result > (ticks_elapsed + 1)) {
            outportw(WS_TIMER_HBL_RELOAD_PORT, result - ticks_elapsed);
        } else {
            continue;
        }

        ws_int_ack(WS_INT_ACK_HBL_TIMER);
        return;
    }
}

static int strlen16(const uint16_t __far* text) {
    int i = 0;
    while (*(text++)) i++;
    return i;
}

int ui_vgmplay(const char *path) {
    vgm_state_t local_vgm_state;
    FIL fp;

    uint8_t vtotal_initial = inportb(WS_LCD_VTOTAL_PORT);

    ui_layout_bars();

    uint8_t result = f_open(&fp, path, FA_READ);
    if (result != FR_OK) {
        return result;
    }

    ui_draw_titlebar_filename(path);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return ERR_FILE_TOO_LARGE;
    }
    result = f_read_rom_banked(&fp, 0, size, NULL, NULL);
    f_close(&fp);
    if (result != FR_OK) {
        return result;
    }

    uint16_t vgm_bank = 0;
    uint16_t vgm_banks_used = (size + 65535L) >> 16;
    memops_unpack_psram_data_if_gzip(&vgm_bank, vgm_banks_used);
    
    ui_draw_statusbar(NULL);
    ui_draw_titlebar(path);

    vgm_state = &local_vgm_state;

    ws_sound_reset();
    outportb(WS_SOUND_WAVE_BASE_PORT, WS_SOUND_WAVE_BASE_ADDR(0x3FC0));

    ui_unload_wallpaper();

    if (!vgm_init(&local_vgm_state, vgm_bank, 0)) {
        ws_sound_reset();
        return ERR_FILE_FORMAT_INVALID;
    }

    // parse GD3 data
    ws_bank_with_rom0(vgm_bank, {
        uint32_t gd3_offset = *((uint32_t __far*) MK_FP(WS_ROM0_SEGMENT, 0x0014));
        if (gd3_offset != 0) {
            gd3_offset += 0x20;
            ws_bank_rom0_set(vgm_bank + (gd3_offset >> 16));
            ws_bank_rom1_set(vgm_bank + (gd3_offset >> 16) + 1);
            const uint16_t __far* gd3_data = MK_FP(WS_ROM0_SEGMENT + ((gd3_offset & 0xFFF0) >> 4), (gd3_offset & 0xF));

            int x = 8;
            int y = 16;
            for (int i = 0; i < 8; i++) {
                int len = strlen16(gd3_data);

                if (len > 0 && (i == 0 || i == 2 || i == 4 || i == 6)) {
                    // Print text
                    bitmapfont_set_active_font(i == 0 ? font16_bitmap : font8_bitmap);
                    bitmapfont_draw_string16(&ui_bitmap, x, y, gd3_data, 248);
                    y += i == 0 ? 16 : 12;
                }

                gd3_data += len + 1;
            }

        }
    });

    input_wait_clear();
    outportb(WS_SOUND_OUT_CTRL_PORT, WS_SOUND_OUT_CTRL_SPEAKER_ENABLE | WS_SOUND_OUT_CTRL_HEADPHONE_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_VOLUME_100);

    outportw(WS_TIMER_HBL_RELOAD_PORT, 2);
    outportb(WS_TIMER_CTRL_PORT, WS_TIMER_CTRL_HBL_REPEAT);

    ws_int_set_handler(WS_INT_HBL_TIMER, (ia16_int_handler_t) vgm_interrupt_handler);
    ws_int_enable(WS_INT_ENABLE_HBL_TIMER);

    while (vgm_state->bank != 0xFF) {
        input_update();
        if (input_pressed) {
            break;
        }
    }

    ws_int_disable(WS_INT_ENABLE_HBL_TIMER | WS_INT_ENABLE_LINE_MATCH);
    outportb(WS_TIMER_CTRL_PORT, 0);
    ws_sound_reset();

    if (inportb(WS_LCD_VTOTAL_PORT) != vtotal_initial) {
        lcd_set_vtotal(vtotal_initial);
    }

    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
    ui_init();
    settings_load();

    return 0;
}
