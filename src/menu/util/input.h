/**
 * Copyright (c) 2022, 2023, 2024 Adrian Siekierka
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

#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include <ws.h>

extern uint16_t input_pressed, input_held, input_released;

#define KEY_UP WS_KEY_X1
#define KEY_DOWN WS_KEY_X3
#define KEY_LEFT WS_KEY_X4
#define KEY_RIGHT WS_KEY_X2

#define KEY_AUP WS_KEY_Y1
#define KEY_ADOWN WS_KEY_Y3
#define KEY_ALEFT WS_KEY_Y4
#define KEY_ARIGHT WS_KEY_Y2

#define KEY_PCV2_VIEW (1 << 12)

void vblank_input_update(void);
void input_reset(void);
void input_update(void);
void input_wait_clear(void);
void input_wait_key(uint16_t key);
void input_wait_any_key(void);

#endif /* _INPUT_H_ */
