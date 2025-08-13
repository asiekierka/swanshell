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

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>
#include "vgm.h"

#define VGM_SAMPLES_TO_LINES(x) (((((uint32_t) (x)) * 120) + 440) / 441)

bool vgm_init_sn76489(vgm_state_t *state, uint8_t __far *header);
uint16_t vgm_cmd_driver_sn76489(vgm_state_t *state, uint8_t cmd);

uint16_t vgm_cmd_driver_ws(vgm_state_t *state, uint8_t cmd);