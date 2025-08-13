/**
 * Copyright (c) 2025 Adrian Siekierka
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

#include <wonderful.h>
#include <ws.h>
#include "util.h"

extern uint8_t __wf_heap_start;

uint16_t mem_query_free(void) {
    return ia16_get_sp() - (uint16_t) &__wf_heap_start;
}

void lcd_set_vtotal(uint8_t vtotal) {
    while (inportb(WS_DISPLAY_LINE_PORT) >= 96);
    while (inportb(WS_DISPLAY_LINE_PORT) < 16);
    outportb(WS_LCD_VTOTAL_PORT, vtotal);
    outportb(WS_LCD_STN_VSYNC_PORT, vtotal - 3);
}