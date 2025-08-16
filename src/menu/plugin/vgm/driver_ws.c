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

// return: amount of HBLANK lines to wait
uint16_t vgm_cmd_driver_ws(vgm_state_t *state, uint8_t cmd) {
    switch (cmd) {
        case 0xBC: { /* WonderSwan - write byte */
            uint8_t addr = *(state->ptr++);
            uint8_t data = *(state->ptr++);
            switch (addr) {
            case 0x0F: // skip wavebase
                break;
            case 0x11: // skip output control
                break;
            case 0x10: // special handling for Ch2 DMA
                if (!(data & 0x20))
                    outportb(WS_SDMA_CTRL_PORT, 0);
                outportb(0x80 + addr, data);
                break;
            default:
                outportb(0x80 + addr, data);
                break;
            }
        } return 0;
        case 0xC6: { /* WonderSwan - write memory */
            uint8_t ofshi = *(state->ptr++);
            uint8_t ofslo = *(state->ptr++);
            uint8_t data = *(state->ptr++);
            uint16_t addr = (((ofshi << 8) | ofslo) & 0x3F) + (inportb(WS_SOUND_WAVE_BASE_PORT) << 6);
            *((uint8_t*) addr) = data;
        } return 0;
        default: return VGM_PLAYBACK_FINISHED;
    }
}
