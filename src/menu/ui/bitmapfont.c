/**
 * Copyright (c) 2024, 2025, 2026 Adrian Siekierka
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

#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <wsx/utf8.h>
#include <nilefs.h>
#include "bitmap.h"
#include "config.h"
#include "strings.h"
#include "util/file.h"
#include "util/math.h"
#include "util/types.h"
#include "../../../build/menu/build/font_tiny8_bin.h"
#include "../../../build/menu/build/font_tiny16_bin.h"

uint8_t font_banks[2] = {0xFF, 0xFF};
uint16_t font_offsets[2] = {
    __builtin_ia16_FP_OFF(font_tiny8),
    __builtin_ia16_FP_OFF(font_tiny16)
};
uint8_t active_font = font16_bitmap;

void bitmapfont_set_active_font(uint16_t font) {
    active_font = font;
}

// __-prefixed functions corrupt/do not use ROM0; non-__-prefixed preserve/do not use ROM0

static inline bitmapfont_header_t __far* bitmapfont_get_header_ptr(void) {
    return MK_FP(WS_ROM0_SEGMENT, font_offsets[active_font]);
}

static const uint16_t __far* __bitmapfont_find_char(uint32_t ch) {
    uint16_t ch_high = ch >> 8;
    
    ws_bank_rom0_set(font_banks[active_font]);
    bitmapfont_header_t __far* header = bitmapfont_get_header_ptr();

    if (ch_high > header->max_codepoint_8)
        return NULL;

    uint8_t ch_low = ch & 0xFF;
    farptr_t __far* base_ptr = ((farptr_t __far*) MK_FP(WS_ROM0_SEGMENT, font_offsets[active_font] + header->lut_offset + 3 * ch_high));
    const uint8_t __far* base = MK_FP(WS_ROM0_SEGMENT, font_offsets[active_font] + base_ptr->offset + 2);
    size_t size = 5;
    if (base_ptr->bank) {
        ws_bank_rom0_set(base_ptr->bank + font_banks[active_font]);
    }
    uint16_t nmemb = *((const uint16_t __far*) (base - 2));
    if (nmemb == 0xFFFF) {
        const uint16_t __far* ptr = (const uint16_t __far*) (base + (ch_low << 2));
        if (*ptr == 0)
            return NULL;
        return ptr;
    }

    const uint8_t __far* pivot;
    size_t corr;

    while (nmemb) {
        /* algorithm needs -1 correction if remaining elements are an even number. */
        corr = nmemb & 1;
        nmemb >>= 1;
        pivot = (const uint8_t __far*) ((( const char __far* )base) + ( nmemb * size ));
        if (*pivot < ch_low) {
            base = (const uint8_t __far*) ((( const char __far* )pivot) + size);
            /* applying correction */
            nmemb -= ( 1 - corr );
        } else if (*pivot == ch_low) {
            return (const uint16_t __far*) (pivot + 1);
        }
    }

    return NULL;
}

static const uint16_t __far* __bitmapfont_get_error_glyph(void) {
    ws_bank_rom0_set(font_banks[active_font]);
    bitmapfont_header_t __far* header = bitmapfont_get_header_ptr();
    return ((const uint16_t __far*) (((const uint8_t __far*) header) + header->error_offset));
}

static inline uint16_t __bitmapfont_get_char_width(const uint16_t __far* data16) {
    if (!FP_SEG(data16))
        data16 = __bitmapfont_get_error_glyph();
    
    const uint8_t __far *data = (const uint8_t __far*) data16;
    return (data[2] & 0xF) + (data[3] & 0xF);    
}

static inline uint16_t __bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const uint16_t __far* data) {
    if (!FP_SEG(data))
        data = __bitmapfont_get_error_glyph();

    uint16_t x = data[1] & 0xF;
    uint16_t y = (data[1] >>  4) & 0xF;
    uint16_t w = (data[1] >>  8) & 0xF;
    uint16_t h = (data[1] >> 12) & 0xF;
    if (h) {
        const uint8_t __far* font_data = ((const uint8_t __far*) data) + data[0];
        bitmap_draw_glyph(bitmap, xofs + x, yofs + y, w, h, 0, font_data);
    }

    return x + w;
}

uint16_t bitmapfont_get_font_height(void) {
    ws_bank_with_rom0(font_banks[active_font], {
        return bitmapfont_get_header_ptr()->font_height;
    });
}

uint16_t bitmapfont_get_char_width(uint32_t ch) {
    if (ch < 0x20) {
        return 0;
    }
    ws_bank_with_rom0(font_banks[active_font], {
        return __bitmapfont_get_char_width(__bitmapfont_find_char(ch));
    });
}

uint16_t bitmapfont_draw_char(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, uint32_t ch) {
    if (ch < 0x20) {
        return 0;
    }
    return __bitmapfont_draw_char(bitmap, xofs, yofs, __bitmapfont_find_char(ch));
}

uint16_t bitmapfont_get_string_width(const char __far* str, uint16_t max_width) {
    uint32_t ch;
    uint16_t width = 0;

    ws_bank_with_rom0(font_banks[active_font], {
        while ((ch = wsx_utf8_decode_next(&str)) != 0) {
            if (ch < 0x20) {
                continue;
            }
            uint16_t new_width = width + __bitmapfont_get_char_width(__bitmapfont_find_char(ch));
            if (new_width > max_width)
                return width - CONFIG_FONT_CHAR_GAP;

            width = new_width + CONFIG_FONT_CHAR_GAP;
        }
    });

    return width - CONFIG_FONT_CHAR_GAP;
}

uint16_t bitmapfont_draw_string(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t max_width) {
    uint32_t ch;
    uint16_t width = 0;

    ws_bank_with_rom0(font_banks[active_font], {
        while ((ch = wsx_utf8_decode_next(&str)) != 0) {
            if (ch < 0x20) {
                continue;
            }
            const uint16_t __far* data = __bitmapfont_find_char(ch);
            uint16_t new_width = width + __bitmapfont_get_char_width(data);
            if (new_width > max_width)
                return width - CONFIG_FONT_CHAR_GAP;

            uint16_t w = __bitmapfont_draw_char(bitmap, xofs, yofs, data) + CONFIG_FONT_CHAR_GAP;
            xofs += w;
            width += w;
        }
    });

    return width - CONFIG_FONT_CHAR_GAP;
}

uint16_t bitmapfont_draw_string16(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const uint16_t __far* str, uint16_t max_width) {
    // FIXME: UTF-16 support
    uint16_t width = 0;

    ws_bank_with_rom0(font_banks[active_font], {
        while (*str != 0) {
            if (*str < 0x20) {
                str++; continue;
            }
            const uint16_t __far* data = __bitmapfont_find_char(*(str++));
            uint16_t new_width = width + __bitmapfont_get_char_width(data);
            if (new_width > max_width)
                return width - CONFIG_FONT_CHAR_GAP;

            uint16_t w = __bitmapfont_draw_char(bitmap, xofs, yofs, data) + CONFIG_FONT_CHAR_GAP;
            xofs += w;
            width += w;
        }
    });

    return width - CONFIG_FONT_CHAR_GAP;
}

void bitmapfont_get_string_box(const char __far* str, uint16_t *width, uint16_t *height, int linegap) {
    uint32_t ch;
    uint16_t line_width = 0;
    uint16_t max_width = 0;
    const char __far* break_str = NULL;
    uint16_t break_width = 0;

    ws_bank_with_rom0(font_banks[active_font], {
        *height = 0;
        while (true) {
            ch = wsx_utf8_decode_next(&str);
repeat_char:
            ;
            bool is_soft_break = ch == ' ';
            bool is_hard_break = ch == '\n';
            if (is_soft_break && !line_width) {
                continue;
            }
            uint16_t new_line_width = line_width + bitmapfont_get_char_width(ch);
            if (is_soft_break || is_hard_break) {
                break_str = str;
                break_width = line_width;
            }
            if (new_line_width > *width || is_hard_break || !ch) {
                bool char_consumed = false;
                if (new_line_width <= *width) {
                    line_width = new_line_width;
                } else if (break_str != NULL) {
                    str = break_str;
                    line_width = break_width;
                    break_str = NULL;
                    char_consumed = true;
                }
                if (max_width < line_width) {
                    max_width = line_width;
                }
                line_width = 0;
                *height += bitmapfont_get_font_height() + linegap;
                if (!ch) {
                    break;
                } else if (!char_consumed) {
                    goto repeat_char;
                }
            } else {
                line_width = new_line_width + CONFIG_FONT_CHAR_GAP;
            }
        }
    });

    *width = max_width;
    if (*height) *height -= linegap;
}

uint16_t bitmapfont_draw_string_box(const bitmap_t *bitmap, uint16_t xofs, uint16_t yofs, const char __far* str, uint16_t width, int linegap) {
    uint32_t ch;
    uint16_t line_width = 0;
    const char __far* start_str = str;
    const char __far* prev_str = str;
    const char __far* break_str = NULL;
    uint16_t start_yofs = yofs;

    ws_bank_with_rom0(font_banks[active_font], {
        while (true) {
            ch = wsx_utf8_decode_next(&str);
            bool is_soft_break = ch == ' ';
            bool is_hard_break = ch == '\n';
            if (is_soft_break && !line_width) {
                continue;
            }
            uint16_t new_line_width = line_width + bitmapfont_get_char_width(ch);
            if (is_soft_break || is_hard_break) {
                break_str = str;
            }
            if (new_line_width > width || is_hard_break || !ch) {
                if (!ch) {
                    break_str = str;
                } else if (break_str == NULL) {
                    break_str = prev_str;
                }
                uint16_t local_xofs = xofs;
                while (start_str < break_str) {
                    ch = wsx_utf8_decode_next(&start_str);
                    local_xofs += bitmapfont_draw_char(bitmap, local_xofs, yofs, ch) + CONFIG_FONT_CHAR_GAP;
                }
                str = break_str;
                start_str = break_str;
                prev_str = break_str;
                break_str = NULL;
                line_width = 0;
                yofs += bitmapfont_get_font_height() + linegap;
                if (!ch) {
                    break;
                }
            } else {
                line_width = new_line_width + CONFIG_FONT_CHAR_GAP;
                prev_str = str;
            }
        }
    });
    
    return yofs - start_yofs;
}

static int16_t bitmapfont_load_font(uint16_t id, uint16_t end_bank, const char __far *filename) {
    char buf[81];
    int16_t result;
    FIL fp;

    strcpy(buf, filename);
    result = f_open(&fp, buf, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK) return result;

    uint16_t start_bank = end_bank - ((f_size(&fp) + 65535) >> 16);
    result = f_read_rom_banked(&fp, start_bank, f_size(&fp), NULL, NULL);
    f_close(&fp);
    if (result == FR_OK) {
        font_banks[id] = start_bank;
        font_offsets[id] = 0x0000;
    }
    return result;
}

int16_t bitmapfont_load(void) {
    int16_t result;
    uint16_t system_end_bank = 0xF2;

    result = bitmapfont_load_font(font8_bitmap, system_end_bank, s_path_font8);
    if (result != FR_OK) return result;
    system_end_bank = MIN(system_end_bank, font_banks[font8_bitmap]);
    result = bitmapfont_load_font(font16_bitmap, font_banks[font8_bitmap], s_path_font16);
    return result;
}