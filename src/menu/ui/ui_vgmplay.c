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
#include <ws/hardware.h>
#include "fatfs/ff.h"
#include "lang.h"
#include "ui.h"
#include "../util/input.h"
#include "../main.h"
#include "../util/vgm.h"

static vgmswan_state_t *vgm_state;

void  __attribute__((interrupt)) vgm_interrupt_handler(void) {
    while (true) {
        uint16_t result = vgmswan_play(vgm_state);
        if (result > 1) {
            if (result != VGMSWAN_PLAYBACK_FINISHED) {
                outportw(IO_TIMER_CTRL, 0);
                outportw(IO_HBLANK_TIMER, result);
                outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE);
            } else {
                vgm_state->bank = 0xFF;
            }
            ws_hwint_ack(HWINT_HBLANK_TIMER);
            return;
        }
    }
}

void ui_vgmplay(const char *path) {
    vgmswan_state_t local_vgm_state;
    FIL fp;

    ui_layout_bars();

	uint8_t result = f_open(&fp, path, FA_READ);
	if (result != FR_OK) {
        // TODO
        return;
	}

    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys_en[LK_UI_STATUS_LOADING]);

	outportb(IO_CART_FLASH, CART_FLASH_ENABLE);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return;
    }
    uint16_t offset = 0;
    uint16_t bank = 0;

	while (size > 0) {
		outportw(IO_BANK_2003_RAM, bank);
        uint16_t to_read = size > 0x8000 ? 0x8000 : size;
        if ((result = f_read(&fp, MK_FP(0x1000, offset), to_read, NULL)) != FR_OK) {
            goto ui_vgmplay_end;
        }
        size -= to_read;
        if (offset == 0x8000) {
            offset = 0;
            bank++;
        } else {
            offset = 0x8000;
        }
	}
    
	outportb(IO_CART_FLASH, 0);
    outportw(IO_BANK_2003_ROM0, 0);
    if (*((const uint32_t __far*) MK_FP(0x2000, 0x0000)) != 0x206d6756) {
        goto ui_vgmplay_end;
    }

    ui_draw_statusbar(NULL);
    ui_draw_titlebar(path);

    vgm_state = &local_vgm_state;
    vgmswan_init(&local_vgm_state, 0, 0);
    outportb(IO_SND_WAVE_BASE, SND_WAVE_BASE(0x2000));
    outportb(IO_SND_OUT_CTRL, 0x0F);

    outportw(IO_HBLANK_TIMER, 2);
    outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE);

    ws_hwint_set_handler(HWINT_IDX_HBLANK_TIMER, vgm_interrupt_handler);
    ws_hwint_enable(HWINT_HBLANK_TIMER);

    while (vgm_state->bank != 0xFF) {
        input_update();
        if (input_pressed) {
            break;
        }
    }

    ws_hwint_disable(HWINT_HBLANK_TIMER);
    outportw(IO_TIMER_CTRL, 0);
    outportb(IO_SND_OUT_CTRL, 0);
    outportb(IO_SND_CH_CTRL, 0);

ui_vgmplay_end:
    f_close(&fp);
    ui_init();
}
