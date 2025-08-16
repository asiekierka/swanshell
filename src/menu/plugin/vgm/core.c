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
#include "plugin/vgm/vgm.h"
#include "vgm_internal.h"

void dprint(const char __far* format, ...);

static uint8_t __far* vgm_state_to_ptr(vgm_state_t *state, uint16_t *backup) {
    if (backup != NULL) *backup = inportw(WS_CART_BANK_ROM0_PORT);

    outportb(WS_CART_BANK_ROM0_PORT, state->bank);
    outportb(WS_CART_BANK_ROM1_PORT, state->bank + 1);

    return MK_FP(0x2000 | (state->pos >> 4), state->pos & 0xF);
}

static void vgm_ptr_to_state(vgm_state_t *state, uint8_t __far* ptr) {
    uint16_t pos_1 = (FP_SEG(ptr) << 4);
    uint16_t pos_2 = FP_OFF(ptr);
    state->pos = pos_1 + pos_2;
    if (state->pos < pos_1) {
        state->bank++;
    }
}

static uint32_t vgm_get_offset(uint8_t __far* ptr) {
    uint32_t version = ((uint32_t __far*) ptr)[2];
    uint16_t offset = 0x40;
    if (version >= 0x150) {
        offset = *((uint16_t __far*) (ptr + 0x34)) + 0x34;
    }
    return offset;
}

static void vgm_jump_to_start_point(vgm_state_t *state) {
    uint16_t bank_backup;

    state->bank = state->start_bank;
    state->pos = state->start_pos;

    uint8_t __far* ptr = vgm_state_to_ptr(state, &bank_backup);
    
    uint16_t offset = vgm_get_offset(ptr);
    state->pos += offset;
    
    outportw(WS_CART_BANK_ROM0_PORT, bank_backup);
}

static void vgm_jump_to_loop_point(vgm_state_t *state) {
    uint16_t bank_backup;

    state->bank = state->start_bank;
    state->pos = state->start_pos;

    uint8_t __far* ptr = vgm_state_to_ptr(state, &bank_backup);

    uint16_t offset = vgm_get_offset(ptr);
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
    
    outportw(WS_CART_BANK_ROM0_PORT, bank_backup);
}

bool vgm_init(vgm_state_t *state, uint8_t bank, uint16_t pos) {
    uint16_t bank_backup;
    
    memset(state, 0, sizeof(vgm_state_t));
    state->start_bank = bank;
    state->start_pos = pos;
    state->bank = bank;
    state->pos = pos;

    uint16_t detected_systems = 0;
    bool error = false;
    uint8_t __far* ptr = vgm_state_to_ptr(state, &bank_backup);

    // check header
    if (((uint32_t __far*) ptr)[0] != 0x206d6756) { error = true; goto on_error; }
    // check supported version
    uint32_t version = ((uint32_t __far*) ptr)[2];
    if (version > 0x172) { error = true; goto on_error; }
    // check clocks
    uint32_t sn76489_clock = ((uint32_t __far*) ptr)[3];
    if (sn76489_clock) {
        state->clock = sn76489_clock;
        state->cmd_driver = vgm_cmd_driver_sn76489;
        if (version >= 0x151) {
            state->sn76489.flags = ptr[0x2B];
        }

        if (sn76489_clock & 0x80000000) { error = true; goto on_error; }
        
        if (!vgm_init_sn76489(state, ptr)) { error = true; goto on_error; }
        detected_systems++;
    }

    if (version >= 0x161) {
        uint32_t dmg_clock = ((uint32_t __far*) ptr)[32];
        if (dmg_clock) {
            state->clock = dmg_clock;
            state->cmd_driver = vgm_cmd_driver_dmg;

            if (!vgm_init_dmg(state, ptr)) { error = true; goto on_error; }
            detected_systems++;
        }
    }

    if (version >= 0x171) {
        uint32_t ws_clock = ((uint32_t __far*) ptr)[48];
        if (ws_clock) {
            state->clock = ws_clock;
            state->cmd_driver = vgm_cmd_driver_ws;

            detected_systems++;
        }
    }
    uint16_t offset = vgm_get_offset(ptr);
    // check unsupported clocks
    if (((uint32_t __far*) ptr)[4]) { error = true; goto on_error; }
    if (version >= 0x110 && ((uint32_t __far*) ptr)[11]) { error = true; goto on_error; }
    if (version >= 0x110 && ((uint32_t __far*) ptr)[12]) { error = true; goto on_error; }
    if (version >= 0x151 && ((uint32_t __far*) ptr)[14]) { error = true; goto on_error; }
    if (offset > 16*4 && version >= 0x151 && ((uint32_t __far*) ptr)[16]) { error = true; goto on_error; }
    if (offset > 17*4 && version >= 0x151 && ((uint32_t __far*) ptr)[17]) { error = true; goto on_error; }
    if (offset > 18*4 && version >= 0x151 && ((uint32_t __far*) ptr)[18]) { error = true; goto on_error; }
    if (offset > 19*4 && version >= 0x151 && ((uint32_t __far*) ptr)[19]) { error = true; goto on_error; }
    if (offset > 20*4 && version >= 0x151 && ((uint32_t __far*) ptr)[20]) { error = true; goto on_error; }
    if (offset > 21*4 && version >= 0x151 && ((uint32_t __far*) ptr)[21]) { error = true; goto on_error; }
    if (offset > 22*4 && version >= 0x151 && ((uint32_t __far*) ptr)[22]) { error = true; goto on_error; }
    if (offset > 23*4 && version >= 0x151 && ((uint32_t __far*) ptr)[23]) { error = true; goto on_error; }
    if (offset > 24*4 && version >= 0x151 && ((uint32_t __far*) ptr)[24]) { error = true; goto on_error; }
    if (offset > 25*4 && version >= 0x151 && ((uint32_t __far*) ptr)[25]) { error = true; goto on_error; }
    if (offset > 26*4 && version >= 0x151 && ((uint32_t __far*) ptr)[26]) { error = true; goto on_error; }
    if (offset > 27*4 && version >= 0x151 && ((uint32_t __far*) ptr)[27]) { error = true; goto on_error; }
    if (offset > 28*4 && version >= 0x151 && ((uint32_t __far*) ptr)[28]) { error = true; goto on_error; }
    if (offset > 29*4 && version >= 0x151 && ((uint32_t __far*) ptr)[29]) { error = true; goto on_error; }
    if (offset > 33*4 && version >= 0x161 && ((uint32_t __far*) ptr)[33]) { error = true; goto on_error; }
    if (offset > 34*4 && version >= 0x161 && ((uint32_t __far*) ptr)[34]) { error = true; goto on_error; }
    if (offset > 35*4 && version >= 0x161 && ((uint32_t __far*) ptr)[35]) { error = true; goto on_error; }
    if (offset > 36*4 && version >= 0x161 && ((uint32_t __far*) ptr)[36]) { error = true; goto on_error; }
    if (offset > 38*4 && version >= 0x161 && ((uint32_t __far*) ptr)[38]) { error = true; goto on_error; }
    if (offset > 39*4 && version >= 0x161 && ((uint32_t __far*) ptr)[39]) { error = true; goto on_error; }
    if (offset > 40*4 && version >= 0x161 && ((uint32_t __far*) ptr)[40]) { error = true; goto on_error; }
    if (offset > 41*4 && version >= 0x161 && ((uint32_t __far*) ptr)[41]) { error = true; goto on_error; }
    if (offset > 42*4 && version >= 0x161 && ((uint32_t __far*) ptr)[42]) { error = true; goto on_error; }
    if (offset > 43*4 && version >= 0x161 && ((uint32_t __far*) ptr)[43]) { error = true; goto on_error; }
    if (offset > 44*4 && version >= 0x161 && ((uint32_t __far*) ptr)[44]) { error = true; goto on_error; }
    if (offset > 45*4 && version >= 0x161 && ((uint32_t __far*) ptr)[45]) { error = true; goto on_error; }
    if (offset > 46*4 && version >= 0x171 && ((uint32_t __far*) ptr)[46]) { error = true; goto on_error; }
    if (offset > 49*4 && version >= 0x171 && ((uint32_t __far*) ptr)[49]) { error = true; goto on_error; }
    if (offset > 50*4 && version >= 0x171 && ((uint32_t __far*) ptr)[50]) { error = true; goto on_error; }
    if (offset > 51*4 && version >= 0x171 && ((uint32_t __far*) ptr)[51]) { error = true; goto on_error; }
    if (offset > 52*4 && version >= 0x171 && ((uint32_t __far*) ptr)[52]) { error = true; goto on_error; }
    if (offset > 54*4 && version >= 0x171 && ((uint32_t __far*) ptr)[54]) { error = true; goto on_error; }
    if (offset > 55*4 && version >= 0x171 && ((uint32_t __far*) ptr)[55]) { error = true; goto on_error; }
    if (offset > 56*4 && version >= 0x171 && ((uint32_t __far*) ptr)[56]) { error = true; goto on_error; }
    if (offset > 57*4 && version >= 0x172 && ((uint32_t __far*) ptr)[57]) { error = true; goto on_error; }

    vgm_jump_to_start_point(state);

#ifdef VGM_USE_PCM
    state->pcm_data_block_location = 0x8000;
#endif
    
on_error:
    outportw(WS_CART_BANK_ROM0_PORT, bank_backup);
    return !error && state->cmd_driver && detected_systems == 1;
}

// return: amount of HBLANK lines to wait
uint16_t vgm_play(vgm_state_t *state) {
    uint16_t bank_backup;
    uint16_t hblanks = 0;
    state->ptr = vgm_state_to_ptr(state, &bank_backup);

    while (hblanks == 0) {
        // play routine! <3
        uint8_t cmd = *(state->ptr++);
        switch (cmd) {
        case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B:
        case 0x7C: case 0x7D: case 0x7E: case 0x7F: {
            hblanks = (cmd & 0x0F) + 1;
        } break;
        case 0x61: {
            uint8_t d1 = *(state->ptr++);
            uint8_t d2 = *(state->ptr++);
            hblanks = ((d2 << 8) | d1);
        } break;
        case 0x62: {
            hblanks = 735;
        } break;
        case 0x63: {
            hblanks = 882;
        } break;
#ifdef VGM_USE_PCM
        case 0x67: { /* PCM data blocks */
            state->ptr++; // 0x66
            state->ptr++; // type - TODO: handle that?
            uint32_t len = *((uint32_t __far*) state->ptr); state->ptr += 4;
try_copy_pcm:
            if (ws_system_is_color_active()) {
                if (state->pcm_data_block_count >= VGM_MAX_DATA_BLOCKS) {
                    hblanks = VGM_PLAYBACK_FINISHED; break;
                }
                // copy data
                uint16_t ofs = state->pcm_data_block_location;
                uint16_t max_ofs;
                uint16_t next_ofs = 0xFFFF;
                if (ofs >= 0x8000) {
                    max_ofs = 0xFE00;
                    next_ofs = 0x1800;
                } else {
                    outportw(WS_DISPLAY_CTRL_PORT, 0);
                    max_ofs = (ws_system_is_color_active() ? 0x8000 : 0x4000);
                }
                if (((uint32_t)ofs + len) > max_ofs) {
                    if (next_ofs != 0xFFFF) {
                        state->pcm_data_block_location = next_ofs;
                        goto try_copy_pcm;
                    }
                    // can't add this much PCM data
                    hblanks = VGM_PLAYBACK_FINISHED; break;
                }
                // dprint("%04X %d %d %d", state->pcm_data_block_location, state->pcm_data_block_count, ofs, len);
                memcpy(MK_FP(0, ofs), state->ptr, len);
                state->pcm_data_block_offset[state->pcm_data_block_count] = ofs;
                state->pcm_data_block_length[state->pcm_data_block_count] = len;
                state->pcm_data_block_location += len;
                state->pcm_data_block_count++;
            }
            state->ptr += len;
        } break;
        case 0x90: { /* Setup Stream Control */
            uint8_t stream_id = *(state->ptr++);
            if (stream_id >= VGM_MAX_STREAMS) {
                // dprint("unsup id %d\n", stream_id);
            }
            state->ptr++; // chip type
            /* state->pcm_stream_ctrl_a[stream_id] = 0x80 + *(state->ptr++);
            state->pcm_stream_ctrl_d[stream_id] = *(state->ptr++); */
            state->ptr += 2;
        } break;
        case 0x91: { /* Set Stream Data? */
            state->ptr += 4;
        } break;
        case 0x92: { /* Set Stream Frequency */
            uint8_t stream_id = *(state->ptr++);
            uint16_t frequency = *((uint16_t __far*) state->ptr); state->ptr += 4;
            
            if (!ws_system_is_color_active()) break;
            state->pcm_stream_flags[stream_id] &= 0xFC;
            if (frequency >= 16000) {
                state->pcm_stream_flags[stream_id] |= VGM_DAC_FREQ_24000;
            } else if (frequency >= 8000) {
                state->pcm_stream_flags[stream_id] |= VGM_DAC_FREQ_12000;
            } else if (frequency >= 4800) {
                state->pcm_stream_flags[stream_id] |= VGM_DAC_FREQ_6000;
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
            uint8_t stream_id = *(state->ptr++);
            uint32_t offset = *((uint32_t __far*) state->ptr); state->ptr += 4;
            uint8_t flags = *(state->ptr++);
            uint32_t length = *((uint32_t __far*) state->ptr); state->ptr += 4;

            if (!ws_system_is_color_active()) break;

            uint32_t location = 0x8000;
            if (offset != 0xFFFFFFFF) location += offset;
            if (location >= 0x10000) {
                hblanks = VGM_PLAYBACK_FINISHED; break;
            }

            outportb(WS_SDMA_CTRL_PORT, 0);
            outportw(WS_SDMA_SOURCE_L_PORT, location);
            outportb(WS_SDMA_SOURCE_H_PORT, location >> 16);
            switch (flags & 0x03) {
            case 0: break;
            case 3:
            case 1: {
                // TODO: number of commands not supported
                outportw(WS_SDMA_LENGTH_L_PORT, length);
                outportb(WS_SDMA_LENGTH_H_PORT, 0);
            } break;
            case 2: {
                outportw(WS_SDMA_LENGTH_L_PORT, length * 44);
                outportb(WS_SDMA_LENGTH_H_PORT, 0);                
            } break;
            }
            outportb(WS_SDMA_CTRL_PORT, (state->pcm_stream_flags[stream_id] & 0x03) | ((flags & 0x80) ? WS_SDMA_CTRL_REPEAT : 0) | WS_SDMA_CTRL_ENABLE);
            // outportb(state->pcm_stream_ctrl_a[stream_id], state->pcm_stream_ctrl_d[stream_id]);
        } break;
        case 0x94: { /* Stop Stream */
            state->ptr++;
            
            if (!ws_system_is_color_active()) break;
            outportb(WS_SDMA_CTRL_PORT, 0);
        } break;
        case 0x95: { /* Start Stream Fast */
            uint8_t stream_id = *(state->ptr++);
            uint8_t block_id = *(state->ptr); state->ptr += 2;
            uint8_t flags = *(state->ptr++);
            
            if (!ws_system_is_color_active()) break;
            outportb(WS_SDMA_CTRL_PORT, 0);
            outportw(WS_SDMA_SOURCE_L_PORT, state->pcm_data_block_offset[block_id]);
            outportb(WS_SDMA_SOURCE_H_PORT, 0);
            outportw(WS_SDMA_LENGTH_L_PORT, state->pcm_data_block_length[block_id]);
            outportb(WS_SDMA_LENGTH_H_PORT, 0);
            outportb(WS_SDMA_CTRL_PORT, (state->pcm_stream_flags[stream_id] & 0x03) | ((flags & 0x01) ? WS_SDMA_CTRL_REPEAT : 0) | WS_SDMA_CTRL_ENABLE);
            // outportb(state->pcm_stream_ctrl_a[stream_id], state->pcm_stream_ctrl_d[stream_id]);

            // dprint("%d %d %d %d %d", stream_id, flags, block_id, state->pcm_stream_flags[stream_id], length);
        } break;
#else
        case 0x67: { /* PCM data blocks */
            state->ptr++; // 0x66
            state->ptr++; // type - TODO: handle that?
            uint32_t len = *((uint32_t __far*) state->ptr); state->ptr += 4;
            state->ptr += len;
        } break;
        case 0x90: { /* Setup Stream Control */
            state->ptr += 4;
        } break;
        case 0x91: { /* Set Stream Data */
            state->ptr += 4;
        } break;
        case 0x92: { /* Set Stream Frequency */
            state->ptr += 5;
        } break;
        case 0x93: { /* Start Stream */
            state->ptr += 10;
        } break;
        case 0x94: { /* Stop Stream */
            state->ptr++;
        } break;
        case 0x95: { /* Start Stream Fast */
            state->ptr += 4;
        } break;
#endif
        default: {
            hblanks = state->cmd_driver(state, cmd);
        } break;
        case 0x66: {
            // hblanks = VGM_PLAYBACK_FINISHED;
            vgm_jump_to_loop_point(state);
            outportw(WS_CART_BANK_ROM0_PORT, bank_backup);
            return 0;
        } break;
        }
    }

    vgm_ptr_to_state(state, state->ptr);
    outportw(WS_CART_BANK_ROM0_PORT, bank_backup);
    return VGM_SAMPLES_TO_LINES(hblanks);
}
