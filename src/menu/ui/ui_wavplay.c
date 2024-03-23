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
#include "ui.h"
#include "../util/input.h"
#include "../main.h"

#define WAV_BUFFER_SIZE 8192
#define WAV_BUFFER_LINEAR0 0x06000
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
void ui_wavplay_irq(void);

void ui_wavplay(const char *path) {
    FIL fp;
    wave_fmt_t fmt;
    uint8_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) {
        // TODO
        return;
    }

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

    if (fmt.format != 1 || fmt.channels != 1 || fmt.bits_per_sample != 8) {
        // TODO
        goto ui_wavplay_end;
    }

    uint16_t timer_step = 0;
    if (fmt.sample_rate > 12000 || fmt.sample_rate < 1) {
        // TODO
        goto ui_wavplay_end;
    }
    timer_step = 12000 / fmt.sample_rate;
    uint16_t timer_sample_rate = 12000 / timer_step;

    POSITION_COUNTER = 0;
    POSITION_COUNTER_INCR = ((uint32_t) fmt.sample_rate << 18) / timer_sample_rate;

    if ((result = f_read(&fp, WAV_BUFFER0, WAV_BUFFER_SIZE * 2, NULL)) != FR_OK) {
        // TODO
        goto ui_wavplay_end;
    }

    outportw(IO_DISPLAY_CTRL, 0);

    input_wait_clear();

    outportb(IO_SND_CH_CTRL, 0);
    outportb(IO_SND_OUT_CTRL, 0);
    outportb(IO_SND_VOL_CH2, 0x80);
    outportb(IO_SND_CH_CTRL, SND_CH2_ENABLE | SND_CH2_VOICE);
    outportb(IO_SND_VOL_CH2_VOICE, SND_VOL_CH2_FULL);
    outportb(IO_SND_OUT_CTRL, SND_OUT_HEADPHONES_ENABLE | SND_OUT_SPEAKER_ENABLE | SND_OUT_DIVIDER_2);

    cpu_irq_disable();
    ws_hwint_set_handler(HWINT_IDX_HBLANK_TIMER, ui_wavplay_irq);
    outportw(IO_HBLANK_TIMER, timer_step);
    outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_REPEAT);
    ws_hwint_enable(HWINT_HBLANK_TIMER);
    cpu_irq_enable();

    uint8_t next_buffer = 0;
    while (true) {
        unsigned int bytes_read;
        if (next_buffer == 1) {
            if (!(POSITION_COUNTER_HIGH & 0x8000)) {
                if ((result = f_read(&fp, WAV_BUFFER1, WAV_BUFFER_SIZE, &bytes_read)) != FR_OK || bytes_read < WAV_BUFFER_SIZE) {
                    // TODO
                    break;
                }
                next_buffer = 0;
            }
        } else {
            if (POSITION_COUNTER_HIGH & 0x8000) {
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

    ws_hwint_disable(HWINT_HBLANK_TIMER);
    outportw(IO_TIMER_CTRL, 0);
    outportb(IO_SND_OUT_CTRL, 0);
    outportb(IO_SND_CH_CTRL, 0);
    
ui_wavplay_end:
    f_close(&fp);
    ui_init();
}
