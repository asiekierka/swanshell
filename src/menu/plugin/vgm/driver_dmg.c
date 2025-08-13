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

#include <string.h>
#include "vgm.h"
#include "vgm_internal.h"
#include <ws.h>
#include "../../main.h"
#include "../../util/util.h"

static vgm_state_t *vstate;

static const uint8_t __far ch4_clock_divider[] = {
    1, 2, 4, 6, 8, 10, 12, 14
};
static const uint8_t __far ch3_volume[] = {
    0, 15, 7, 3
};
static const uint8_t __far duty_waveforms[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,

    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,

    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

static inline void dmg_sync_period(vgm_state_t *state, uint8_t idx) {
    uint8_t shift = 2;
    uint16_t divider = (1536000L * (2048 - (state->dmg.period[idx] & 0x7FF))) / (state->clock >> shift);
    outportw(0x80 + (idx << 1), -divider);
}

static void dmg_sync_volume(vgm_state_t *state, uint8_t idx) {
    uint8_t mvol_left = (state->dmg.mvol >> 4) & 0x7;
    uint8_t mvol_right = state->dmg.mvol & 0x7;
    mvol_right *= (state->dmg.pan >> idx) & 1;
    mvol_left *= (state->dmg.pan >> (4 + idx)) & 1;
    uint8_t ch_vol = state->dmg.c_volume[idx];

    outportb(0x88 + idx, ((ch_vol * mvol_right + 6) >> 3) | (((ch_vol * mvol_left + 6) >> 3) << 4));
}

__attribute__((always_inline))
static inline void dmg_tick_envelope(uint8_t ch_active, uint8_t idx) {
    uint8_t en = vstate->dmg.en[idx];
    if ((ch_active & (1 << idx)) && (en & 7)) {
        vstate->dmg.c_envelope[idx]++;
        if (vstate->dmg.c_envelope[idx] == (en & 7)) {
            if (en & 8) {
                if (vstate->dmg.c_volume[idx] < 15) {
                    vstate->dmg.c_volume[idx]++;
                    dmg_sync_volume(vstate, idx);
                }
            } else {
                if (vstate->dmg.c_volume[idx] > 0) {
                    vstate->dmg.c_volume[idx]--;
                    dmg_sync_volume(vstate, idx);
                }
            }

            vstate->dmg.c_envelope[idx] = 0;
        }
    }
}

// Faux-256 Hz timer.
__attribute__((assume_ss_data, interrupt))
static void __far dmg_line_int_handler(void) {
    uint8_t ch_active = inportb(WS_SOUND_CH_CTRL_PORT);

    // Length timers
    if ((ch_active & 0x01) && (vstate->dmg.period1hi & 0x40)) {
        if (vstate->dmg.c_len[0]) {
            vstate->dmg.c_len[0] = (vstate->dmg.c_len[0] + 1) & 0x3F;
        }
        if (!vstate->dmg.c_len[0]) {
            ch_active &= ~0x01;
        }
    }
    if ((ch_active & 0x02) && (vstate->dmg.period2hi & 0x40)) {
        if (vstate->dmg.c_len[1]) {
            vstate->dmg.c_len[1] = (vstate->dmg.c_len[1] + 1) & 0x3F;
        }
        if (!vstate->dmg.c_len[1]) {
            ch_active &= ~0x02;
        }
    }
    if ((ch_active & 0x04) && (vstate->dmg.period3hi & 0x40)) {
        if (vstate->dmg.c_len[2]) {
            vstate->dmg.c_len[2]++;
        }
        if (!vstate->dmg.c_len[2]) {
            ch_active &= ~0x04;
        }
    }
    if ((ch_active & 0x08) && (vstate->dmg.go4 & 0x40)) {
        if (vstate->dmg.c_len[3]) {
            vstate->dmg.c_len[3] = (vstate->dmg.c_len[3] + 1) & 0x3F;
        }
        if (!vstate->dmg.c_len[3]) {
            ch_active &= ~0x08;
        }
    }

    // Envelope timers
    if (!(vstate->dmg.tick & 3)) {
        dmg_tick_envelope(ch_active, 0);
        dmg_tick_envelope(ch_active, 1);
        dmg_tick_envelope(ch_active, 3);
    }

    // Sweep timer
    if (!(vstate->dmg.tick & 1)) {
        int16_t period = vstate->dmg.period[0];
        int16_t period_change = period >> (vstate->dmg.sweep1 & 7);
        int16_t new_period = (vstate->dmg.sweep1 & 8) ? (period - period_change) : (period + period_change);
        if (vstate->dmg.sweep1 & 0x70) {
            if (new_period >= 0x800) {
                ch_active = ~0x01;
            }
            vstate->dmg.c_sweep++;
            if (vstate->dmg.c_sweep == ((vstate->dmg.sweep1 >> 4) & 0x7)) {
                if (new_period < 0) new_period = 0;
                vstate->dmg.period[0] = new_period;
                dmg_sync_period(vstate, 0);
                vstate->dmg.c_sweep = 0;
            }
        }
    }

    outportb(WS_SOUND_CH_CTRL_PORT, ch_active);

    switch (inportb(WS_DISPLAY_LINE_IRQ_PORT)) {
    case 0:  outportb(WS_DISPLAY_LINE_IRQ_PORT, 47); break;
    case 47: outportb(WS_DISPLAY_LINE_IRQ_PORT, 95); break;
    case 95: outportb(WS_DISPLAY_LINE_IRQ_PORT, 142); break;
    default: outportb(WS_DISPLAY_LINE_IRQ_PORT, 0); break;
    }
    vstate->dmg.tick++;
    ws_int_ack(WS_INT_ACK_LINE_MATCH);
}

bool vgm_init_dmg(vgm_state_t *state, uint8_t __far *header) {
    vstate = state;
    lcd_set_vtotal(188);

    outportb(WS_DISPLAY_LINE_IRQ_PORT, 10);
    ws_int_set_handler(WS_INT_LINE_MATCH, dmg_line_int_handler);
    ws_int_enable(WS_INT_ENABLE_LINE_MATCH);

    return true;
}

static inline void dmg_sync_duty(vgm_state_t *state, uint8_t idx) {
    memcpy(
        MK_FP(0x0000, (inportb(WS_SOUND_WAVE_BASE_PORT) << 6) + (idx << 4)),
        duty_waveforms + ((state->dmg.len[idx] >> 6) << 4),
        16
    );
}

static void dmg_sync_all_volumes(vgm_state_t *state) {
    dmg_sync_volume(state, 0);
    dmg_sync_volume(state, 1);
    dmg_sync_volume(state, 2);
    dmg_sync_volume(state, 3);
}

static inline void dmg_trigger(vgm_state_t *state, uint8_t idx) {
    // reset length timer
    if (!state->dmg.c_len[idx]) {
        if (idx == 2) {
            state->dmg.c_len[idx] = state->dmg.len[idx];
        } else {
            state->dmg.c_len[idx] = state->dmg.len[idx] & 0x3F;
        }
    }
    // set volume
    if (idx == 2) {
        state->dmg.c_volume[idx] = ch3_volume[(state->dmg.level3 >> 5) & 3];
    } else {
        state->dmg.c_volume[idx] = state->dmg.en[idx] >> 4;
    }
    dmg_sync_volume(state, idx);
    // enable channel
    state->dmg.c_envelope[idx] = 0;
    uint8_t ch_mask = 1 << idx;
    if (idx == 0) {
        state->dmg.c_sweep = 0;
    } else if (idx == 3) {
        ch_mask |= 0x80;
        outportb(WS_SOUND_NOISE_CTRL_PORT, inportb(WS_SOUND_NOISE_CTRL_PORT) | WS_SOUND_NOISE_CTRL_RESET);
    }
    outportb(WS_SOUND_CH_CTRL_PORT, inportb(WS_SOUND_CH_CTRL_PORT) | ch_mask);
}

uint16_t vgm_cmd_driver_dmg(vgm_state_t *state, uint8_t cmd) {
    switch (cmd) {
        case 0xB3: {
            uint8_t addr = *(state->ptr++);
            uint8_t data = *(state->ptr++);
            switch (addr) {
                case 0x16:
                    outportb(WS_SOUND_OUT_CTRL_PORT, (data >> 7) * (WS_SOUND_OUT_CTRL_HEADPHONE_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_ENABLE | WS_SOUND_OUT_CTRL_SPEAKER_VOLUME_100));
                    if (!(data & 0x80)) {
                        memset(&state->dmg, 0, sizeof(state->dmg));
                    }
                    break;
                case 0x15:
                    state->dmg.pan = data;
                    dmg_sync_all_volumes(state);
                    break;
                case 0x14:
                    state->dmg.mvol = data;
                    dmg_sync_all_volumes(state);
                    break;
                case 0x00:
                    state->dmg.sweep1 = data;
                    break;
                case 0x01:
                    state->dmg.len[0] = data;
                    dmg_sync_duty(state, 0);
                    break;
                case 0x02:
                    state->dmg.en[0] = data;
                    break;
                case 0x03:
                    state->dmg.period1lo = data;
                    dmg_sync_period(state, 0);
                    break;
                case 0x04:
                    state->dmg.period1hi = data;
                    dmg_sync_period(state, 0);
                    if (data & 0x80) dmg_trigger(state, 0);
                    break;
                case 0x06:
                    state->dmg.len[1] = data;
                    dmg_sync_duty(state, 1);
                    break;
                case 0x07:
                    state->dmg.en[1] = data;
                    break;
                case 0x08:
                    state->dmg.period2lo = data;
                    dmg_sync_period(state, 1);
                    break;
                case 0x09:
                    state->dmg.period2hi = data;
                    dmg_sync_period(state, 1);
                    if (data & 0x80) dmg_trigger(state, 1);
                    break;
                case 0x0A:
                    state->dmg.en[2] = data;
                    break;
                case 0x0B:
                    state->dmg.len[2] = data;
                    break;
                case 0x0C:
                    state->dmg.level3 = data;
                    break;
                case 0x0D:
                    state->dmg.period3lo = data;
                    dmg_sync_period(state, 2);
                    break;
                case 0x0E:
                    state->dmg.period3hi = data;
                    dmg_sync_period(state, 2);
                    if (data & 0x80) dmg_trigger(state, 2);
                    break;
                case 0x10:
                    state->dmg.len[3] = data & 0x3F;
                    break;
                case 0x11:
                    state->dmg.en[3] = data;
                    break;
                case 0x12:
                    outportb(WS_SOUND_NOISE_CTRL_PORT, WS_SOUND_NOISE_CTRL_ENABLE | (data & 0x8 ? WS_SOUND_NOISE_CTRL_LENGTH_254 : WS_SOUND_NOISE_CTRL_LENGTH_32767));
                    uint32_t clock = (ch4_clock_divider[data & 7] << (data >> 4));
                    uint16_t divider = 1500 * clock / (state->clock >> 15);
                    outportw(0x86, -divider);
                    break;
                case 0x13:
                    state->dmg.go4 = data;
                    if (data & 0x80) dmg_trigger(state, 3);
                    break;
                case 0x20: case 0x21: case 0x22: case 0x23:
                case 0x24: case 0x25: case 0x26: case 0x27:
                case 0x28: case 0x29: case 0x2A: case 0x2B:
                case 0x2C: case 0x2D: case 0x2E: case 0x2F: // Wave RAM
                    ((uint8_t*) (inportb(WS_SOUND_WAVE_BASE_PORT) << 6))[addr] = (data >> 4) | (data << 4);
                    break;
            }
        } return 0;
        default: return VGM_PLAYBACK_FINISHED;
    }
}