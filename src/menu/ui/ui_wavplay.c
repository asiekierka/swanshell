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
#include <ws/system.h>
#include "fatfs/ff.h"
#include "../../../build/menu/assets/menu/lang.h"
#include "ui.h"
#include "../util/input.h"
#include "../main.h"

#define WAV_BUFFER_SIZE 8192
#define WAV_BUFFER_LINEAR0 0x0A000
#define WAV_BUFFER_LINEAR1 (WAV_BUFFER_LINEAR0 + WAV_BUFFER_SIZE)
#define WAV_BUFFER0 MK_FP(WAV_BUFFER_LINEAR0 >> 16, WAV_BUFFER_LINEAR0 & 0xFFFF)
#define WAV_BUFFER1 MK_FP(WAV_BUFFER_LINEAR1 >> 16, WAV_BUFFER_LINEAR1 & 0xFFFF)

#define RIFF_CHUNK_RIFF 0x46464952
#define RIFF_CHUNK_WAVE 0x45564157
#define RIFF_CHUNK_fmt  0x20746D66
#define RIFF_CHUNK_data 0x61746164

#define POSITION_COUNTER_INCR *((volatile uint32_t*) 0x18)
#define POSITION_COUNTER *((volatile uint32_t*) 0x1C)
#define POSITION_COUNTER_HIGH *((volatile uint16_t*) 0x1E)

typedef struct {
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wave_fmt_t;

__attribute__((interrupt))
void ui_wavplay_irq_8_mono(void);

void ui_wavplay(const char *path) {
    FIL fp;
    wave_fmt_t fmt;

    if (!ws_system_color_active()) {
        return;
    }

    ui_layout_bars();

    uint8_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) {
        // TODO
        return;
    }

    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys_en[LK_UI_STATUS_LOADING]);

    memset(WAV_BUFFER0, 0, WAV_BUFFER_SIZE * 2);
    fmt.channels = 0;

    uint32_t chunk_info[3];
    if ((result = f_read(&fp, chunk_info, 12, NULL)) != FR_OK) {
        // TODO
        goto ui_wavplay_end;
    }
    if (chunk_info[0] != RIFF_CHUNK_RIFF || chunk_info[2] != RIFF_CHUNK_WAVE) {
        // TODO
        goto ui_wavplay_end;
    }
    
    while (true) {
        if ((result = f_read(&fp, chunk_info, 8, NULL)) != FR_OK) {
            // TODO
            goto ui_wavplay_end;
        }
        uint32_t remainder = f_tell(&fp) + chunk_info[1];

        if (chunk_info[0] == RIFF_CHUNK_data) {
            if (fmt.channels == 0) {
                // TODO: fmt not found
                goto ui_wavplay_end;
            }
            break;
        } else if (chunk_info[0] == RIFF_CHUNK_fmt) {
            if ((result = f_read(&fp, &fmt, sizeof(fmt), NULL)) != FR_OK) {
                // TODO
                goto ui_wavplay_end;
            }
        }

        if ((result = f_lseek(&fp, remainder)) != FR_OK) {
            // TODO
            goto ui_wavplay_end;
        }
    }

    if (fmt.format != 1) {
        // TODO
        goto ui_wavplay_end;
    }

    if ((result = f_read(&fp, WAV_BUFFER0, WAV_BUFFER_SIZE * 2, NULL)) != FR_OK) {
        // TODO
        goto ui_wavplay_end;
    }

    ui_draw_statusbar(NULL);
    ui_draw_titlebar(path);

    outportb(IO_SND_CH_CTRL, 0);
    outportb(IO_SND_OUT_CTRL, 0);

    input_wait_clear();

    if (fmt.sample_rate >= 65536 || fmt.sample_rate < 1) {
        // TODO
        goto ui_wavplay_end;
    }
    
    bool use_irq = true;
    if (ws_system_color_active()) {
        if (fmt.channels == 1 && fmt.bits_per_sample == 8)
            if (fmt.sample_rate == 24000 || fmt.sample_rate == 12000 || fmt.sample_rate == 6000 || fmt.sample_rate == 4000)
                use_irq = false;
    }

    if (use_irq) {
        if (fmt.channels != 1 || fmt.bits_per_sample != 8) {
            // TODO
            goto ui_wavplay_end;
        }

        uint16_t timer_step = 0;
        timer_step = fmt.sample_rate > 12000 ? 1 : 12000 / fmt.sample_rate;
        uint16_t timer_sample_rate = 12000 / timer_step;

        POSITION_COUNTER = 0;
        POSITION_COUNTER_INCR = (((uint32_t) fmt.sample_rate << 16) / timer_sample_rate) << 2;

        outportb(IO_SND_VOL_CH2, 0x80);
        outportb(IO_SND_CH_CTRL, SND_CH2_ENABLE | SND_CH2_VOICE);
        outportb(IO_SND_VOL_CH2_VOICE, SND_VOL_CH2_FULL);
        outportb(IO_SND_OUT_CTRL, SND_OUT_HEADPHONES_ENABLE | SND_OUT_SPEAKER_ENABLE | SND_OUT_DIVIDER_2);

        cpu_irq_disable();
        ws_hwint_set_handler(HWINT_IDX_HBLANK_TIMER, ui_wavplay_irq_8_mono);
        outportw(IO_HBLANK_TIMER, timer_step);
        outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_REPEAT);
        ws_hwint_enable(HWINT_HBLANK_TIMER);
        cpu_irq_enable();
    } else {
        outportb(IO_SND_VOL_CH2, 0x80);
        outportb(IO_SND_CH_CTRL, SND_CH2_ENABLE | SND_CH2_VOICE);
        outportb(IO_SND_VOL_CH2_VOICE, SND_VOL_CH2_FULL);
        outportb(IO_SND_OUT_CTRL, SND_OUT_HEADPHONES_ENABLE | SND_OUT_SPEAKER_ENABLE | SND_OUT_DIVIDER_2);

        outportw(IO_SDMA_SOURCE_L, WAV_BUFFER_LINEAR0);
        outportb(IO_SDMA_SOURCE_H, 0);
        outportw(IO_SDMA_LENGTH_L, WAV_BUFFER_SIZE * 2);
        outportb(IO_SDMA_LENGTH_H, 0);

        uint8_t rate;
        if (fmt.sample_rate == 24000)
            rate = 3;
        else if (fmt.sample_rate == 12000)
            rate = 2;
        else if (fmt.sample_rate == 6000)
            rate = 1;
        else
            rate = 0;

        outportb(IO_SDMA_CTRL, DMA_TRANSFER_ENABLE | rate | SDMA_REPEAT | SDMA_TARGET_CH2);
    }
    
    uint8_t next_buffer = 0;
    while (true) {
        unsigned int bytes_read;
        bool read_next = false;
        if (next_buffer == 1) {
            if (use_irq) {
                read_next = POSITION_COUNTER_HIGH < WAV_BUFFER_LINEAR1;
            } else {
                read_next = inportw(IO_SDMA_SOURCE_L) < WAV_BUFFER_LINEAR1;
            }
            if (read_next) {
                if ((result = f_read(&fp, WAV_BUFFER1, WAV_BUFFER_SIZE, &bytes_read)) != FR_OK || bytes_read < WAV_BUFFER_SIZE) {
                    // TODO
                    break;
                }
                next_buffer = 0;
            }
        } else {
            if (use_irq) {
                read_next = POSITION_COUNTER_HIGH >= WAV_BUFFER_LINEAR1;
            } else {
                read_next = inportw(IO_SDMA_SOURCE_L) >= WAV_BUFFER_LINEAR1;
            }
            if (read_next) {
                if ((result = f_read(&fp, WAV_BUFFER0, WAV_BUFFER_SIZE, &bytes_read)) != FR_OK || bytes_read < WAV_BUFFER_SIZE) {
                    // TODO
                    break;
                }
                next_buffer = 1;
            }
        }

        input_update();
        if (input_pressed) {
            break;
        }
    }

    if (use_irq) {
        ws_hwint_disable(HWINT_HBLANK_TIMER);
        outportw(IO_TIMER_CTRL, 0);
    } else {
        outportb(IO_SDMA_CTRL, 0);
    }
    outportb(IO_SND_OUT_CTRL, 0);
    outportb(IO_SND_CH_CTRL, 0);
    
ui_wavplay_end:
    f_close(&fp);
    ui_init();
}
