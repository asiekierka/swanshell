/**
 * WonderSwan audio playback library
 *
 * Copyright (c) 2022 Adrian "asie" Siekierka
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

#include <stddef.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/hardware.h>
#include <ws/system.h>
#include "vgm.h"

#define VGM_SAMPLES_TO_LINES(x) (((((uint32_t) (x)) * 120) + 440) / 441)

void dprint(const char __far* format, ...);

static uint8_t __far* vgmswan_state_to_ptr(vgmswan_state_t *state, uint16_t *backup) {
    if (backup != NULL) *backup = inportw(IO_BANK_ROM0);

    outportb(IO_BANK_ROM0, state->bank);
    outportb(IO_BANK_ROM1, state->bank + 1);

    return MK_FP(0x2000 | (state->pos >> 4), state->pos & 0xF);
}

static void vgmswan_ptr_to_state(vgmswan_state_t *state, uint8_t __far* ptr) {
    uint16_t pos_1 = (FP_SEG(ptr) << 4);
    uint16_t pos_2 = FP_OFF(ptr);
    state->pos = pos_1 + pos_2;
    if (state->pos < pos_1) {
        state->bank++;
    }
}

static void vgmswan_jump_to_start_point(vgmswan_state_t *state) {
    uint16_t bank_backup;

    state->bank = state->start_bank;
    state->pos = state->start_pos;

    uint8_t __far* ptr = vgmswan_state_to_ptr(state, &bank_backup);

    uint16_t offset = *((uint16_t __far*) (ptr + 0x34)) + 0x34;
    state->pos += offset;
    
    outportw(IO_BANK_ROM0, bank_backup);
}

static void vgmswan_jump_to_loop_point(vgmswan_state_t *state) {
    uint16_t bank_backup;

    state->bank = state->start_bank;
    state->pos = state->start_pos;

    uint8_t __far* ptr = vgmswan_state_to_ptr(state, &bank_backup);

    uint16_t offset = *((uint16_t __far*) (ptr + 0x34)) + 0x34;
    state->pos += offset;

    uint32_t loop_point = *((uint32_t __far*) (ptr + 0x1C));
    if (loop_point == 0 || (loop_point & 0xFF000000)) {
        // no loop point
    } else {
        // loop point
        loop_point += 0x1C;
        state->pos = loop_point;
        state->bank = (loop_point >> 16) + state->bank;
    }
    
    outportw(IO_BANK_ROM0, bank_backup);
}

void vgmswan_init(vgmswan_state_t *state, uint8_t bank, uint16_t pos) {
    memset(state, 0, sizeof(vgmswan_state_t));
    state->start_bank = bank;
    state->start_pos = pos;
    vgmswan_jump_to_start_point(state);

#ifdef VGMSWAN_USE_PCM
    state->pcm_data_block_location = 0x8000;
#endif
}

// return: amount of HBLANK lines to wait
uint16_t vgmswan_play(vgmswan_state_t *state) {
    uint16_t bank_backup;
    uint16_t result = 0;
    uint8_t __far* ptr = vgmswan_state_to_ptr(state, &bank_backup);
    while (result == 0) {
        // play routine! <3
        uint8_t cmd = *(ptr++);
        if ((cmd & 0xF0) == 0x70) {
            result = (cmd & 0x0F) + 1;
        } else switch (cmd) {
        case 0x61: {
            uint8_t d1 = *(ptr++);
            uint8_t d2 = *(ptr++);
            result = ((d2 << 8) | d1);
        } break;
        case 0x62: {
            result = 735;
        } break;
        case 0x63: {
            result = 882;
        } break;
#ifdef VGMSWAN_USE_PCM
        case 0x67: { /* PCM data blocks */
            ptr++; // 0x66
            ptr++; // type - TODO: handle that?
            uint32_t len = *((uint32_t __far*) ptr); ptr += 4;
try_copy_pcm:
            if (ws_system_color_active()) {
                if (state->pcm_data_block_count >= VGMSWAN_MAX_DATA_BLOCKS) {
                    result = VGMSWAN_PLAYBACK_FINISHED; break;
                }
                // copy data
                uint16_t ofs = state->pcm_data_block_location;
                uint16_t max_ofs;
                uint16_t next_ofs = 0xFFFF;
                if (ofs >= 0x8000) {
                    max_ofs = 0xFE00;
                    next_ofs = 0x1800;
                } else {
                    outportw(IO_DISPLAY_CTRL, 0);
                    max_ofs = (ws_system_color_active() ? 0x8000 : 0x4000);
                }
                if (((uint32_t)ofs + len) > max_ofs) {
                    if (next_ofs != 0xFFFF) {
                        state->pcm_data_block_location = next_ofs;
                        goto try_copy_pcm;
                    }
                    // can't add this much PCM data
                    result = VGMSWAN_PLAYBACK_FINISHED; break;
                }
                // dprint("%04X %d %d %d", state->pcm_data_block_location, state->pcm_data_block_count, ofs, len);
                memcpy(MK_FP(0, ofs), ptr, len);
                state->pcm_data_block_offset[state->pcm_data_block_count] = ofs;
                state->pcm_data_block_length[state->pcm_data_block_count] = len;
                state->pcm_data_block_location += len;
                state->pcm_data_block_count++;
            }
            ptr += len;
        } break;
        case 0x90: { /* Setup Stream Control */
            uint8_t stream_id = *(ptr++);
            if (stream_id >= VGMSWAN_MAX_STREAMS) {
                // dprint("unsup id %d\n", stream_id);
            }
            ptr++; // chip type
            /* state->pcm_stream_ctrl_a[stream_id] = 0x80 + *(ptr++);
            state->pcm_stream_ctrl_d[stream_id] = *(ptr++); */
            ptr += 2;
        } break;
        case 0x91: { /* Set Stream Data? */
            ptr += 4;
        } break;
        case 0x92: { /* Set Stream Frequency */
            uint8_t stream_id = *(ptr++);
            uint16_t frequency = *((uint16_t __far*) ptr); ptr += 4;
            
            if (!ws_system_color_active()) break;
            state->pcm_stream_flags[stream_id] &= 0xFC;
            if (frequency >= 16000) {
                state->pcm_stream_flags[stream_id] |= VGMSWAN_DAC_FREQ_24000;
            } else if (frequency >= 8000) {
                state->pcm_stream_flags[stream_id] |= VGMSWAN_DAC_FREQ_12000;
            } else if (frequency >= 4800) {
                state->pcm_stream_flags[stream_id] |= VGMSWAN_DAC_FREQ_6000;
            }
            /* switch (frequency) {
            case 4000: break;
            case 6000: break;
            case 12000: break;
            case 24000: break;
            default: dprint("unsup freq %d", frequency);
            } */
        } break;
        case 0x93: { /* Start Stream - TODO */
            uint8_t stream_id = *(ptr++);
            uint32_t offset = *((uint32_t __far*) ptr); ptr += 4;
            uint8_t flags = *(ptr++);
            uint32_t length = *((uint32_t __far*) ptr); ptr += 4;

            if (!ws_system_color_active()) break;

            uint32_t location = 0x8000;
            if (offset != 0xFFFFFFFF) location += offset;
            if (location >= 0x10000) {
                result = VGMSWAN_PLAYBACK_FINISHED; break;
            }

            outportb(IO_SDMA_CTRL, 0);
            outportw(IO_SDMA_SOURCE_L, location);
            outportb(IO_SDMA_SOURCE_H, location >> 16);
            switch (flags & 0x03) {
            case 0: break;
            case 3:
            case 1: {
                // TODO: number of commands not supported
                outportw(IO_SDMA_LENGTH_L, length);
                outportb(IO_SDMA_LENGTH_H, 0);
            } break;
            case 2: {
                outportw(IO_SDMA_LENGTH_L, length * 44);
                outportb(IO_SDMA_LENGTH_H, 0);                
            } break;
            }
            outportb(IO_SDMA_CTRL, (state->pcm_stream_flags[stream_id] & 0x03) | ((flags & 0x80) ? SDMA_REPEAT : 0) | DMA_TRANSFER_ENABLE);
            // outportb(state->pcm_stream_ctrl_a[stream_id], state->pcm_stream_ctrl_d[stream_id]);
        } break;
        case 0x94: { /* Stop Stream */
            ptr++;
            
            if (!ws_system_color_active()) break;
            outportb(IO_SDMA_CTRL, 0);
        } break;
        case 0x95: { /* Start Stream Fast */
            uint8_t stream_id = *(ptr++);
            uint8_t block_id = *(ptr); ptr += 2;
            uint8_t flags = *(ptr++);
            
            if (!ws_system_color_active()) break;
            outportb(IO_SDMA_CTRL, 0);
            outportw(IO_SDMA_SOURCE_L, state->pcm_data_block_offset[block_id]);
            outportb(IO_SDMA_SOURCE_H, 0);
            outportw(IO_SDMA_LENGTH_L, state->pcm_data_block_length[block_id]);
            outportb(IO_SDMA_LENGTH_H, 0);
            outportb(IO_SDMA_CTRL, (state->pcm_stream_flags[stream_id] & 0x03) | ((flags & 0x01) ? SDMA_REPEAT : 0) | DMA_TRANSFER_ENABLE);
            // outportb(state->pcm_stream_ctrl_a[stream_id], state->pcm_stream_ctrl_d[stream_id]);

            // dprint("%d %d %d %d %d", stream_id, flags, block_id, state->pcm_stream_flags[stream_id], length);
        } break;
#else
        case 0x67: { /* PCM data blocks */
            ptr++; // 0x66
            ptr++; // type - TODO: handle that?
            uint32_t len = *((uint32_t __far*) ptr); ptr += 4;
            ptr += len;
        } break;
        case 0x90: { /* Setup Stream Control */
            ptr += 4;
        } break;
        case 0x91: { /* Set Stream Data */
            ptr += 4;
        } break;
        case 0x92: { /* Set Stream Frequency */
            ptr += 5;
        } break;
        case 0x93: { /* Start Stream */
            ptr += 10;
        } break;
        case 0x94: { /* Stop Stream */
            ptr++;
        } break;
        case 0x95: { /* Start Stream Fast */
            ptr += 4;
        } break;
#endif
#ifdef VGMSWAN_MODE_WONDERSWAN
        case 0xBC: { /* WonderSwan - write byte */
            uint8_t addr = *(ptr++);
            uint8_t data = *(ptr++);
            if (addr == 0x0F || addr == 0x11) {
                // dprint("skipping wavebase = %02X", data);
            } else {
                if (addr == 0x10 && !(data & 0x20)) {
                    outportb(IO_SDMA_CTRL, 0);
                }
                outportb(0x80 + addr, data);
            }
        } break;
        case 0xC6: { /* WonderSwan - write memory */
            uint8_t ofshi = *(ptr++);
            uint8_t ofslo = *(ptr++);
            uint8_t data = *(ptr++);
            uint16_t addr = ((ofshi << 8) | ofslo) + (inportb(IO_SND_WAVE_BASE) << 6);
            *((uint8_t*) addr) = data;
        } break;
#endif
        case 0x66: {
            // result = VGMSWAN_PLAYBACK_FINISHED;
            vgmswan_jump_to_loop_point(state);
            outportw(IO_BANK_ROM0, bank_backup);
            return 0;
        } break;
        default: {
            // uint16_t off = (uint16_t) ptr;
            // uint16_t seg = FP_SEG(ptr);
            // dprint("unknown cmd %02X %04X:%04X", (uint16_t) cmd, seg, off);
            result = VGMSWAN_PLAYBACK_FINISHED;
        } break;
        }
    }

    vgmswan_ptr_to_state(state, ptr);
    outportw(IO_BANK_ROM0, bank_backup);
    return VGM_SAMPLES_TO_LINES(result);
}
