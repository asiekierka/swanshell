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

#ifndef VGM_H_
#define VGM_H_

#include <stdbool.h>
#include <stdint.h>

#define VGM_DAC_FREQ_4000  0x00
#define VGM_DAC_FREQ_6000  0x01
#define VGM_DAC_FREQ_12000 0x02
#define VGM_DAC_FREQ_24000 0x03
#define VGM_DAC_FREQ_MASK  0x03

#define VGM_USE_PCM
#define VGM_MAX_STREAMS 1
#define VGM_MAX_DATA_BLOCKS 16

struct vgm_state;

typedef uint16_t (*vgm_cmd_driver_t)(struct vgm_state*, uint8_t);

typedef struct vgm_state {
    uint8_t bank, start_bank;
    uint16_t pos, start_pos;
    uint8_t flags;
    vgm_cmd_driver_t cmd_driver;

    uint8_t __far *ptr;

    uint32_t clock;
    union {
        struct {
            uint8_t volume[4];
            uint16_t tone[3];
            uint8_t noise;
            uint8_t latch;
            uint8_t flags;
            uint8_t stereo;
        } sn76489;
    };

#ifdef VGM_USE_PCM
    uint8_t pcm_data_block_count;
    uint16_t pcm_data_block_location;
    uint16_t pcm_data_block_offset[VGM_MAX_DATA_BLOCKS + 1];
    uint16_t pcm_data_block_length[VGM_MAX_DATA_BLOCKS + 1];

    /* uint8_t pcm_stream_ctrl_a[VGM_MAX_STREAMS];
    uint8_t pcm_stream_ctrl_d[VGM_MAX_STREAMS]; */
    uint8_t pcm_stream_flags[VGM_MAX_STREAMS];
#endif
} vgm_state_t;

#define VGM_PLAYBACK_FINISHED 0xFFFF

bool vgm_init(vgm_state_t *state, uint8_t bank, uint16_t pos);
// return: amount of HBLANK lines to wait
uint16_t vgm_play(vgm_state_t *state);

#endif /* VGM_H_ */