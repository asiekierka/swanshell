/**
 * Copyright (c) 2024 Adrian Siekierka
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
 
#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>
#include "utf8.h"

char *utf8_encode_char(char *s, uint32_t chr) {
	if (chr < 0x80) {
		*(s++) = chr;
	} else if (chr < 0x7FF) {
		*(s++) = (chr >> 6) | 0xC0;
		*(s++) = (chr & 0x3F) | 0x80;
	} else if (chr < 0xFFFF) {
		*(s++) = (chr >> 12) | 0xE0;
		*(s++) = ((chr >> 6) & 0x3F) | 0x80;
		*(s++) = (chr & 0x3F) | 0x80;
	} else {
		*(s++) = (chr >> 18) | 0xF0;
		*(s++) = ((chr >> 12) & 0x3F) | 0x80;
		*(s++) = ((chr >> 6) & 0x3F) | 0x80;
		*(s++) = (chr & 0x3F) | 0x80;
	}
	return s;
}

#if 0

uint32_t utf8_decode_char(const char __far** s) {
	uint8_t c = *((*s)++);
	if (c < 0x80) {
		return c;
	} else if ((c & 0xE0) == 0xC0) {
		uint8_t c2 = *((*s)++);
		return ((c & 0x1F) << 6) | (c2 & 0x3F);
	} else if ((c & 0xF0) == 0xE0) {
		uint8_t c2 = *((*s)++);
		uint8_t c3 = *((*s)++);
		return (c << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
	} else {
		uint8_t c2 = *((*s)++);
		uint8_t c3 = *((*s)++);
		uint8_t c4 = *((*s)++);
		return (c << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
	}
}

#endif
