/**
 * Copyright (c) 2024 Adrian Siekierka
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
#include "../util/input.h"
#include "../util/vgm.h"

static vgmswan_state_t *vgm_state;

void  __attribute__((interrupt, assume_ss_data)) vgm_interrupt_handler(void) {
    while (true) {
        uint16_t result = vgmswan_play(vgm_state);
        if (result > 1) {
            if (result != VGMSWAN_PLAYBACK_FINISHED) {
                outportw(WS_TIMER_CTRL_PORT, 0);
                outportw(WS_TIMER_HBL_RELOAD_PORT, result);
                outportw(WS_TIMER_CTRL_PORT, WS_TIMER_CTRL_HBL_ONESHOT);
            } else {
                vgm_state->bank = 0xFF;
            }
            ws_int_ack(WS_INT_ACK_HBL_TIMER);
            return;
        }
    }
}

int ui_vgmplay(const char *path) {
    vgmswan_state_t local_vgm_state;
    FIL fp;

    ui_layout_bars();

    uint8_t result = f_open(&fp, path, FA_READ);
    if (result != FR_OK) {
        return result;
    }

    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);

    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return ERR_FILE_TOO_LARGE;
    }
    uint16_t offset = 0;
    uint16_t bank = 0;

    while (size > 0) {
        outportw(WS_CART_EXTBANK_RAM_PORT, bank);
        uint16_t to_read = size > 0x8000 ? 0x8000 : size;
        if ((result = f_read(&fp, MK_FP(0x1000, offset), to_read, NULL)) != FR_OK) {
            f_close(&fp);
            return result;
        }
        size -= to_read;
        if (offset == 0x8000) {
            offset = 0;
            bank++;
        } else {
            offset = 0x8000;
        }
    }
    f_close(&fp);
    
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
    outportw(WS_CART_EXTBANK_ROM0_PORT, 0);
    if (*((const uint32_t __far*) MK_FP(0x2000, 0x0000)) != 0x206d6756) {
        return ERR_FILE_FORMAT_INVALID;
    }

    ui_draw_statusbar(NULL);
    ui_draw_titlebar(path);

    vgm_state = &local_vgm_state;
    vgmswan_init(&local_vgm_state, 0, 0);
    outportb(WS_SOUND_WAVE_BASE_PORT, WS_SOUND_WAVE_BASE_ADDR(0x3FC0));
    outportb(WS_SOUND_OUT_CTRL_PORT, WS_SOUND_OUT_CTRL_SPEAKER_ENABLE | WS_SOUND_OUT_CTRL_HEADPHONE_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_VOLUME_100);
    outportw(WS_TIMER_HBL_RELOAD_PORT, 2);
    outportw(WS_TIMER_CTRL_PORT, WS_TIMER_CTRL_HBL_ONESHOT);

    ws_int_set_handler(WS_INT_HBL_TIMER, (ia16_int_handler_t) vgm_interrupt_handler);
    ws_int_enable(WS_INT_ENABLE_HBL_TIMER);

    while (vgm_state->bank != 0xFF) {
        input_update();
        if (input_pressed) {
            break;
        }
    }

    ws_int_disable(WS_INT_ENABLE_HBL_TIMER);
    outportw(WS_TIMER_CTRL_PORT, 0);
    outportb(WS_SOUND_OUT_CTRL_PORT, 0);
    outportb(WS_SOUND_CH_CTRL_PORT, 0);

    ui_init();

    return 0;
}
