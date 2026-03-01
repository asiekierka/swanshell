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

#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include <ws.h>
#include "shell/shell.h"

extern const uint8_t __far util_hex_chars[16];

#define HBL_PROFILE_START { ws_timer_hblank_start_once(65535); }
#define HBL_PROFILE_END { \
    uint16_t __hbl_time = ws_timer_hblank_get_counter(); \
    ws_timer_hblank_disable(); \
    char __hbl_buf[81]; \
    const char *__hbl_name = __FUNCTION__; \
    snprintf(__hbl_buf, 81, "%s():%d %d\r\n", (const char __far*) __hbl_name, __LINE__, __hbl_time ^ 0xFFFF); \
    __hbl_buf[80] = 0; \
    nile_mcu_native_cdc_write_string(__hbl_buf); \
}

/**
 * @return uint16_t The approximate number of free RAM bytes.
 */
uint16_t mem_query_free(void);

void lcd_set_vtotal(uint8_t vtotal);

#endif /* UTIL_H_ */
