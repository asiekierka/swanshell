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
#include "strings.h"
#include "../ui/ui.h"
#include "../util/input.h"
#include "../main.h"
#include "ui/bitmap.h"

#define WAV_BUFFER_SIZE 4096
#define WAV_BUFFER_SHIFT 12
#define WAV_BUFFER_LINEAR0 (!ws_system_is_color_active() ? 0x2000 : 0xC000)
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
#define POSITION_COUNTER_MASK_XOR *((volatile uint8_t*) 0x14)
#define POSITION_COUNTER_START *((volatile uint8_t*) 0x15)
#define POSITION_COUNTER_MASK_AND *((volatile uint8_t*) 0x16)
#define POSITION_COUNTER_MASK_OR *((volatile uint8_t*) 0x17)

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

// 5 + 5 + 2 + 5 + 5 = 22
// 22 + 4 + 22 = 48
#define UI_WAV_DURATION_TEXT0_X 4
#define UI_WAV_DURATION_TEXT1_X (UI_WAV_DURATION_TEXT0_X + 22)
#define UI_WAV_DURATION_TEXT2_X (UI_WAV_DURATION_TEXT1_X + 4)
#define UI_WAV_DURATION_BAR_X 55
#define UI_WAV_DURATION_BAR_Y 126
#define UI_WAV_DURATION_BAR_WIDTH 165
#define UI_WAV_DURATION_BAR_HEIGHT 6
#define UI_WAV_DURATION_TEXT_Y (UI_WAV_DURATION_BAR_Y - 1)

static void seconds_to_buffer(uint16_t seconds, char *seconds_buffer) {
    seconds_buffer[4] = (seconds % 10) + '0';
    seconds_buffer[3] = ((seconds / 10) % 6) + '0';
    seconds_buffer[1] = ((seconds / 60) % 10) + '0';
    seconds_buffer[0] = (seconds / 600) + '0';
}

int ui_wavplay(const char *path) {
    char seconds_buffer[6];
    FIL fp;
    wave_fmt_t fmt;
    uint16_t br;

    ui_layout_bars();

    uint8_t result = f_open(&fp, path, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) {
        return result;
    }

    ui_draw_titlebar(NULL);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);

    memset(WAV_BUFFER0, 0, WAV_BUFFER_SIZE * 2);
    fmt.channels = 0;

    uint32_t chunk_info[3];
    if ((result = f_read(&fp, chunk_info, 12, &br)) != FR_OK) {
        return result;
    }
    if (chunk_info[0] != RIFF_CHUNK_RIFF || chunk_info[2] != RIFF_CHUNK_WAVE) {
        return ERR_FILE_FORMAT_INVALID;
    }

    while (true) {
        if ((result = f_read(&fp, chunk_info, 8, &br)) != FR_OK) {
            f_close(&fp);
            return result;
        }
        uint32_t remainder = f_tell(&fp) + chunk_info[1];

        if (chunk_info[0] == RIFF_CHUNK_data) {
            if (fmt.channels == 0) {
                // fmt chunk not found
                f_close(&fp);
                return ERR_FILE_FORMAT_INVALID;
            }
            break;
        } else if (chunk_info[0] == RIFF_CHUNK_fmt) {
            if ((result = f_read(&fp, &fmt, sizeof(fmt), &br)) != FR_OK) {
                f_close(&fp);
                return result;
            }
        }

        if ((result = f_lseek(&fp, remainder)) != FR_OK) {
            f_close(&fp);
            return result;
        }
    }

    if (fmt.format != 1) {
        f_close(&fp);
        return ERR_FILE_FORMAT_INVALID;
    }

    if (fmt.sample_rate >= 65536 || fmt.sample_rate < 1) {
        f_close(&fp);
        return ERR_FILE_FORMAT_INVALID;
    }
    
    if (!ws_system_is_color_active()) {
        ui_hide();
    }

    if ((result = f_read(&fp, WAV_BUFFER0, WAV_BUFFER_SIZE * 2, &br)) != FR_OK) {
        // TODO
        goto ui_wavplay_end;
    }

    uint8_t bytes_per_sample = (fmt.channels * (fmt.bits_per_sample >> 3));
    uint32_t bytes_per_second = (uint32_t) bytes_per_sample * fmt.sample_rate;
    uint32_t bytes_read_subsecond = WAV_BUFFER_SIZE * 2;
    uint32_t seconds_current = 0;
    uint32_t seconds_in_song = chunk_info[1] / bytes_per_second;

    if (ws_system_is_color_active()) {
        ui_draw_statusbar(NULL);
        ui_draw_titlebar(path);

        bitmap_rect_fill(&ui_bitmap,
            UI_WAV_DURATION_TEXT0_X, UI_WAV_DURATION_TEXT_Y,
            UI_WAV_DURATION_BAR_X - UI_WAV_DURATION_TEXT0_X, 8,
            BITMAP_COLOR_2BPP(MAINPAL_COLOR_WHITE));

        bitmapfont_set_active_font(font8_bitmap);
        seconds_buffer[0] = '/';
        seconds_buffer[1] = 0;
        bitmapfont_draw_string(&ui_bitmap,
            UI_WAV_DURATION_TEXT1_X, UI_WAV_DURATION_TEXT_Y,
            seconds_buffer, 65535);

        seconds_buffer[0] = '0';
        seconds_buffer[1] = '0';
        seconds_buffer[2] = ':';
        seconds_buffer[3] = '0';
        seconds_buffer[4] = '0';
        seconds_buffer[5] = 0;
        bitmapfont_draw_string(&ui_bitmap,
            UI_WAV_DURATION_TEXT0_X, UI_WAV_DURATION_TEXT_Y,
            seconds_buffer, 65535);

        seconds_to_buffer(seconds_in_song, seconds_buffer);
        bitmapfont_draw_string(&ui_bitmap,
            UI_WAV_DURATION_TEXT2_X, UI_WAV_DURATION_TEXT_Y,
            seconds_buffer, 65535);

        bitmap_rect_fill(&ui_bitmap,
            UI_WAV_DURATION_BAR_X, UI_WAV_DURATION_BAR_Y,
            UI_WAV_DURATION_BAR_WIDTH, UI_WAV_DURATION_BAR_HEIGHT,
            BITMAP_COLOR_4BPP(MAINPAL_COLOR_WHITE));
        bitmap_rect_draw(&ui_bitmap,
            UI_WAV_DURATION_BAR_X, UI_WAV_DURATION_BAR_Y,
            UI_WAV_DURATION_BAR_WIDTH, UI_WAV_DURATION_BAR_HEIGHT,
            BITMAP_COLOR_2BPP(MAINPAL_COLOR_BLACK), false);
    }

    outportb(WS_SOUND_CH_CTRL_PORT, 0);
    outportb(WS_SOUND_OUT_CTRL_PORT, 0);

    input_wait_clear();

    bool use_irq = true;
    if (ws_system_is_color_active()) {
        if (fmt.channels == 1 && fmt.bits_per_sample == 8)
            if (fmt.sample_rate == 24000 || fmt.sample_rate == 12000 || fmt.sample_rate == 6000 || fmt.sample_rate == 4000)
                use_irq = false;
    }

    if (use_irq) {
        if ((fmt.channels != 1 && fmt.channels != 2) || (fmt.bits_per_sample != 8 && fmt.bits_per_sample != 16)) {
            // TODO
            goto ui_wavplay_end;
        }

        POSITION_COUNTER_MASK_AND = 0xFF ^ (bytes_per_sample - 1);
        POSITION_COUNTER_MASK_OR = (fmt.bits_per_sample >> 3) - 1;
        POSITION_COUNTER_MASK_XOR = fmt.bits_per_sample == 16 ? 0x80 : 0x00;

        uint16_t timer_step = 0;
        timer_step = fmt.sample_rate > 12000 ? 1 : 12000 / fmt.sample_rate;
        uint16_t timer_sample_rate = 12000 / timer_step;

        POSITION_COUNTER = 0;
        POSITION_COUNTER_INCR = ((((uint32_t) fmt.sample_rate << 16) / timer_sample_rate) << (15 - WAV_BUFFER_SHIFT)) * bytes_per_sample;
        POSITION_COUNTER_START = WAV_BUFFER_LINEAR0 >> 8;

        outportb(WS_SOUND_VOICE_SAMPLE_PORT, 0x80);
        outportb(WS_SOUND_CH_CTRL_PORT, WS_SOUND_CH_CTRL_CH2_ENABLE | WS_SOUND_CH_CTRL_CH2_VOICE);
        outportb(WS_SOUND_VOICE_VOL_PORT, WS_SOUND_VOICE_VOL_LEFT_FULL | WS_SOUND_VOICE_VOL_RIGHT_FULL);
        outportb(WS_SOUND_OUT_CTRL_PORT, WS_SOUND_OUT_CTRL_HEADPHONE_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_VOLUME_400);

        ia16_disable_irq();
        ws_int_set_handler(WS_INT_HBL_TIMER, ui_wavplay_irq_8_mono);
        outportw(WS_TIMER_HBL_RELOAD_PORT, timer_step);
        outportw(WS_TIMER_CTRL_PORT, WS_TIMER_CTRL_HBL_REPEAT);
        ws_int_enable(WS_INT_ENABLE_HBL_TIMER);
        ia16_enable_irq();
    } else {
        outportb(WS_SOUND_VOICE_SAMPLE_PORT, 0x80);
        outportb(WS_SOUND_CH_CTRL_PORT, WS_SOUND_CH_CTRL_CH2_ENABLE | WS_SOUND_CH_CTRL_CH2_VOICE);
        outportb(WS_SOUND_VOICE_VOL_PORT, WS_SOUND_VOICE_VOL_LEFT_FULL | WS_SOUND_VOICE_VOL_RIGHT_FULL);
        outportb(WS_SOUND_OUT_CTRL_PORT, WS_SOUND_OUT_CTRL_HEADPHONE_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_VOLUME_400);

        outportw(WS_SDMA_SOURCE_L_PORT, WAV_BUFFER_LINEAR0);
        outportb(WS_SDMA_SOURCE_H_PORT, 0);
        outportw(WS_SDMA_LENGTH_L_PORT, WAV_BUFFER_SIZE * 2);
        outportb(WS_SDMA_LENGTH_H_PORT, 0);

        uint8_t rate;
        if (fmt.sample_rate == 24000)
            rate = 3;
        else if (fmt.sample_rate == 12000)
            rate = 2;
        else if (fmt.sample_rate == 6000)
            rate = 1;
        else
            rate = 0;

        outportb(WS_SDMA_CTRL_PORT, WS_SDMA_CTRL_ENABLE | rate | WS_SDMA_CTRL_REPEAT | WS_SDMA_CTRL_TARGET_CH2);
    }
    
    uint8_t next_buffer = 0;
    while (true) {
        unsigned int bytes_read = 0;
        bool read_next = false;
        if (next_buffer == 1) {
            if (use_irq) {
                read_next = !(POSITION_COUNTER_HIGH & 0x8000);
            } else {
                read_next = inportw(WS_SDMA_SOURCE_L_PORT) < WAV_BUFFER_LINEAR1;
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
                read_next = (POSITION_COUNTER_HIGH & 0x8000);
            } else {
                read_next = inportw(WS_SDMA_SOURCE_L_PORT) >= WAV_BUFFER_LINEAR1;
            }
            if (read_next) {
                if ((result = f_read(&fp, WAV_BUFFER0, WAV_BUFFER_SIZE, &bytes_read)) != FR_OK || bytes_read < WAV_BUFFER_SIZE) {
                    // TODO
                    break;
                }
                next_buffer = 1;
            }
        }

        if (ws_system_is_color_active()) {
            bool update_seconds = false;
            bytes_read_subsecond += bytes_read;
            while (bytes_read_subsecond > bytes_per_second) {
                seconds_current++;
                bytes_read_subsecond -= bytes_per_second;
                update_seconds = true;
            }

            if (update_seconds) {
                wait_for_vblank();

                bitmap_rect_fill(&ui_bitmap,
                    UI_WAV_DURATION_TEXT0_X, UI_WAV_DURATION_TEXT_Y,
                    UI_WAV_DURATION_TEXT1_X - UI_WAV_DURATION_TEXT0_X, 8,
                    BITMAP_COLOR_2BPP(MAINPAL_COLOR_WHITE));

                uint16_t seconds_to_draw = seconds_current >= 5940 ? 5940 : seconds_current;
                seconds_to_buffer(seconds_to_draw, seconds_buffer);
                bitmapfont_draw_string(&ui_bitmap,
                    UI_WAV_DURATION_TEXT0_X, UI_WAV_DURATION_TEXT_Y,
                    seconds_buffer, 65535);

                uint16_t bar_width = (((uint32_t) seconds_current) * (UI_WAV_DURATION_BAR_WIDTH - 2)) / seconds_in_song;
                bitmap_rect_fill(&ui_bitmap,
                    UI_WAV_DURATION_BAR_X + 1, UI_WAV_DURATION_BAR_Y + 1,
                    bar_width, UI_WAV_DURATION_BAR_HEIGHT - 2,
                    BITMAP_COLOR_4BPP(MAINPAL_COLOR_RED));
            }
        }

        input_update();
        if (input_pressed) {
            break;
        }
    }

    if (use_irq) {
        ws_int_disable(WS_INT_ENABLE_HBL_TIMER);
        outportw(WS_TIMER_CTRL_PORT, 0);
    } else {
        outportb(WS_SDMA_CTRL_PORT, 0);
    }
    outportb(WS_SOUND_OUT_CTRL_PORT, 0);
    outportb(WS_SOUND_CH_CTRL_PORT, 0);

ui_wavplay_end:
    f_close(&fp);

    if (!ws_system_is_color_active())
        ui_init();

    return 0;
}
