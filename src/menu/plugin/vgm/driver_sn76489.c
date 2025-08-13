/**
 * VGM playback library
 *
 * Copyright (c) 2022, 2025 Adrian "asie" Siekierka
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "vgm.h"
#include "vgm_internal.h"
#include <ws.h>

// TODO: Accurate noise emulation?

#define SN_TO_WS_CHANNEL(x) (x)

bool vgm_init_sn76489(vgm_state_t *state, uint8_t __far *header) {
    outportb(WS_SOUND_CH_CTRL_PORT, 0x0F);
    outportb(WS_SOUND_OUT_CTRL_PORT, WS_SOUND_OUT_CTRL_HEADPHONE_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_VOLUME_100);
    outportb(WS_SOUND_NOISE_CTRL_PORT, WS_SOUND_NOISE_CTRL_LENGTH_1953 | WS_SOUND_NOISE_CTRL_ENABLE | WS_SOUND_NOISE_CTRL_RESET);

    uint8_t __far *noise_wavetable_data = MK_FP(0x0000, (inportb(WS_SOUND_WAVE_BASE_PORT) << 6) + (SN_TO_WS_CHANNEL(3) << 4));
    for (int i = 0; i < 16; i++) {
        noise_wavetable_data[i] = (i & 7) ? 0x00 : 0x0F;
    }

    return true;
}

static inline void update_noise_freq(vgm_state_t *state) {
    uint16_t tone;
    switch (state->sn76489.noise & 3) {
    case 0: tone = 0x10; break;
    case 1: tone = 0x20; break;
    case 2: tone = 0x40; break;
    case 3: tone = state->sn76489.tone[2]; break;
    }
    uint16_t divider;
    
    if (state->sn76489.flags & 0x08) {
        divider = (3072000L * tone) / (state->clock >> 8);
    } else {
        divider = (3072000L * tone) / (state->clock >> 5);
    }
    outportw(0x80 + SN_TO_WS_CHANNEL(3) * 2, -divider);
    outportb(WS_SOUND_CH_CTRL_PORT, state->sn76489.noise & 0x4 ? 0x8F : 0x0F);                    
}

uint16_t vgm_cmd_driver_sn76489(vgm_state_t *state, uint8_t cmd) {
    switch (cmd) {
        case 0x50: {
            uint8_t data = *(state->ptr++);
            uint8_t target;
            if (data & 0x80) {
                target = data;
                state->sn76489.latch = data;
            } else {
                target = state->sn76489.latch;
            }

            uint8_t channel = (target >> 5) & 3;
            switch ((target >> 4) & 7) {
                case 1: case 3: case 5: case 7: {
                    // volume
                    state->sn76489.volume[channel] = (data & 0xF) ^ 0xF;
                    // update volume on relevant channel
                    outportb(0x88 + SN_TO_WS_CHANNEL(channel), state->sn76489.volume[channel] * 0x11);
                } break;
                case 6: {
                    // noise
                    state->sn76489.noise = (data & 0x7);
                    update_noise_freq(state);
                    outportb(WS_SOUND_CH_CTRL_PORT, (data & 0x04) ? 0x8F : 0x0F);
                } break;
                case 0: case 2: case 4: {
                    // tone
                    if (data & 0x80) {
                        state->sn76489.tone[channel] = (state->sn76489.tone[channel] & 0x3F0) | (data & 0xF);
                    } else {
                        state->sn76489.tone[channel] = (state->sn76489.tone[channel] & 0xF) | ((data & 0x3F) << 4);
                    }
                    // update frequency on relevant channel
                    uint16_t tone = state->sn76489.tone[channel];
                    if (!tone && state->sn76489.flags & 0x00) {
                        tone = 0x400;
                    }
                    uint8_t __far *wavetable_data = MK_FP(0x0000, (inportb(WS_SOUND_WAVE_BASE_PORT) << 6) + (SN_TO_WS_CHANNEL(channel) << 4));
                    uint16_t divider;
                    if (state->sn76489.flags & 0x08) {
                        divider = (3072000L * tone) / (state->clock >> 4);
                    } else {
                        divider = (3072000L * tone) / (state->clock >> 1);
                    }
                    outportw(0x80 + SN_TO_WS_CHANNEL(channel) * 2, -divider);
                    if (tone > 1) {
                        for (int i = 0; i < 16; i++) {
                            wavetable_data[i] = (i & 4) ? 0xFF : 0x00;
                        }
                    } else {
                        for (int i = 0; i < 16; i++) {
                            wavetable_data[i] = 0xFF;
                        }
                    }
                    if (channel == 2 && (state->sn76489.noise & 3) == 3) {
                        update_noise_freq(state);
                    }
                } break;
            }
        } return 0;
        default: while(1);
    }
}