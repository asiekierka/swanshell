/**
 * Copyright (c) 2026 Adrian Siekierka
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
#include <stdio.h>
#include <ws.h>
#include <nilefs.h>
#include <ws/display.h>
#include <wsx/utf8.h>
#include "errors.h"
#include "lang.h"
#include "../ui/ui.h"
#include "../util/file.h"
#include "../util/input.h"
#include "plugin.h"
#include "strings.h"
#include "ui/bitmap.h"

typedef enum {
    TXT_ENCODING_UTF8,
    TXT_ENCODING_SJIS,
    TXT_ENCODING_OTHER,
    TXT_ENCODING_UNKNOWN
} txt_encoding_t;

#define IS_UTF8_CONT_BYTE(c) ((c) >= 0x80 && (c) < 0xC0)

#define DETECT_ENCODING_SIZE_LIMIT (8*1024)

static txt_encoding_t detect_encoding(uint32_t size) {
    uint16_t size_l = (size > DETECT_ENCODING_SIZE_LIMIT) ? DETECT_ENCODING_SIZE_LIMIT : size;
    bool cannot_be_utf8 = false;
    bool cannot_be_sjis = false;

    ws_bank_rom0_set(0);
    for (uint16_t i = 0; i < size_l; i++) {
        const uint8_t __far *ptr = MK_FP(0x2000, i);
        uint8_t ch = *ptr;
        if (ch >= 0xFD) {
            // Neither UTF-8 nor Shift-JIS use bytes from 0xFD to 0xFF.
            return TXT_ENCODING_OTHER;
        }
        
        // UTF-8 validity checks
        if (!cannot_be_utf8) {
            if (ch >= 0xF0) {
                if (!IS_UTF8_CONT_BYTE(ptr[1]) || !IS_UTF8_CONT_BYTE(ptr[2]) || !IS_UTF8_CONT_BYTE(ptr[3]) || IS_UTF8_CONT_BYTE(ptr[4]))
                    cannot_be_utf8 = true;
                else {
                    i += 3;
                    continue;
                }
            } else if (ch >= 0xE0) {
                if (!IS_UTF8_CONT_BYTE(ptr[1]) || !IS_UTF8_CONT_BYTE(ptr[2]) || IS_UTF8_CONT_BYTE(ptr[3]))
                    cannot_be_utf8 = true;
                else {
                    i += 2;
                    continue;
                }
            } else if (ch >= 0xC0) {
                if (!IS_UTF8_CONT_BYTE(ptr[1]) || IS_UTF8_CONT_BYTE(ptr[2]))
                    cannot_be_utf8 = true;
                else {
                    i += 1;
                    continue;
                }
            } else if (IS_UTF8_CONT_BYTE(ch)) {
                cannot_be_utf8 = true;
            }
        }

        // Shift-JIS validity checks
        if (!cannot_be_sjis) {
            if ((ch >= 0x81 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xEF)) {
                if (ptr[1] < 0x40 || ptr[1] == 0x7F || ptr[1] >= 0xFD)
                    cannot_be_sjis = true;
                else {
                    i += 1;
                    continue;
                }
            }
        }

        if (cannot_be_utf8 && cannot_be_sjis)
            return TXT_ENCODING_OTHER;
    }
    if (cannot_be_utf8)
        return TXT_ENCODING_SJIS;
    else if (cannot_be_sjis)
        return TXT_ENCODING_UTF8;
    else
        return TXT_ENCODING_UNKNOWN;
}

static uint32_t sjis_decode_next(const char __far** s, uint16_t tbl_bank) {
    uint8_t ch = *((*s)++);
    if ((ch >= 0x81 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xEF)) {
        uint8_t chl = *((*s)++);
        if (chl < 0x40 || chl >= 0xFD) {
            return '?';
        }
        uint16_t ch_index = (chl - 0x40);
        if (ch >= 0xE0) {
            ch_index += (ch - 0xE0 + 0x1F) * (0xFD - 0x40);
        } else {
            ch_index += (ch - 0x81) * (0xFD - 0x40);
        }
        ws_bank_rom1_set(tbl_bank);
        const uint16_t __far *tbl = MK_FP(0x3000, 0);
        uint16_t result = tbl[ch_index];
        // FIXME: missing mapping
        if (result == 0x2212)
            return '-';
        return result;
    } else if (ch >= 0xA0 && ch < 0xE0) {
        return 0xFF60 + (ch - 0xA0);
    } else if (ch == 0x7E) {
        return 0x203E;
    } else if (ch < 0x7E) {
        return ch;
    } else {
        return '?';
    }
}

int ui_txtview(const char *path) {
    FIL fp;

    ui_layout_bars();

    int16_t result = f_open(&fp, path, FA_READ);
    if (result != FR_OK) return result;

    ui_draw_titlebar_filename(path);
    ui_draw_statusbar(lang_keys[LK_UI_STATUS_LOADING]);

    uint32_t size = f_size(&fp);
    if (size > 4L*1024*1024) {
        return ERR_FILE_TOO_LARGE;
    }

    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
    result = f_read_rom_banked(&fp, 0, size, NULL, NULL);
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);

    f_close(&fp);
    if (result != FR_OK) return result;

    txt_encoding_t encoding = detect_encoding(size);
    uint16_t tbl_bank = (size + 65535) >> 16;
    if (encoding == TXT_ENCODING_SJIS) {
        char buf[64];
        strcpy(buf, s_path_tbl_shiftjis);
        result = f_open(&fp, buf, FA_READ);
        if (result != FR_OK) return result;
        result = f_read_rom_banked(&fp, tbl_bank, f_size(&fp), NULL, NULL);
        f_close(&fp);
        if (result != FR_OK) return result;
    }

    ui_draw_statusbar(NULL);

    uint32_t file_start_pos = 0;
    uint32_t file_next_pos = 0;
    uint32_t file_last_end_pos = 0;
    bool reader_open = true;
    bool reader_redraw = true;

    while (reader_open) {
        if (reader_redraw) {
            ui_layout_bars();
            reader_redraw = false;
            file_last_end_pos = 0;
        }

        // Display text.
        bitmapfont_set_active_font(font16_bitmap);
        int font_height = bitmapfont_get_font_height();
        int y = 8;
        int x = 1;
        uint32_t file_pos = file_start_pos;
        file_next_pos = 0;
        while (file_pos < size) {
            uint32_t file_prev_pos = file_pos;
            ws_bank_rom0_set(file_pos >> 16);
            ws_bank_rom1_set((file_pos >> 16) + 1);
            const char __far *ptr = MK_FP(0x2000 | ((file_pos >> 4) & 0xFFF), file_pos & 0xF);
            uint32_t ch;
            if (encoding == TXT_ENCODING_SJIS) {
                ch = sjis_decode_next(&ptr, tbl_bank);
            } else {
                ch = wsx_utf8_decode_next(&ptr);
            }
            file_pos = (file_pos & ~0xF) + FP_OFF(ptr);

            if (ch < 0x20) {
                if (ch == '\n') {
                    x = 1;
                    y += font_height;
                    if (!file_next_pos) {
                        file_next_pos = file_pos;
                    }
                    if (y >= (WS_DISPLAY_HEIGHT_PIXELS - 8)) {
                        break;
                    }
                } else if (ch == '\t') {
                    x = (x + 0x11) & ~0xF;
                }
                continue;
            }
            uint16_t chw = bitmapfont_get_char_width(ch);
            if ((x + chw) >= WS_DISPLAY_WIDTH_PIXELS) {
                x = 1;
                y += font_height;
                if (!file_next_pos) {
                    file_next_pos = file_prev_pos;
                }
                if (y >= (WS_DISPLAY_HEIGHT_PIXELS - 8)) {
                    break;
                }
            }
            if (file_pos >= file_last_end_pos)
                bitmapfont_draw_char(&ui_bitmap, x, y, ch);
            x += chw;
        }
        file_last_end_pos = file_pos;

        uint32_t file_drawn_pos = file_start_pos;
        while (file_start_pos == file_drawn_pos) {
            input_wait_any_key();
            if (input_pressed & (WS_KEY_B | WS_KEY_START)) {
                reader_open = false;
                break;
            }
            if (input_pressed & (WS_KEY_X1 | WS_KEY_Y1 | WS_KEY_X4 | WS_KEY_Y4)) {
                if (file_start_pos > 1) {
                    file_start_pos--;
                    while (file_start_pos > 0) {
                        file_start_pos--;
                        ws_bank_rom0_set(file_start_pos >> 16);
                        ws_bank_rom1_set(file_start_pos >> 16);
                        const char __far *ptr = MK_FP(0x2000 | ((file_start_pos >> 4) & 0xFFF), file_start_pos & 0xF);
                        if (*ptr == '\n') {
                            file_start_pos++;
                            break;
                        }
                    }
                    reader_redraw = true;
                }
            } else if (input_pressed & (WS_KEY_X2 | WS_KEY_Y2)) {
                if (file_pos < size) {
                    file_start_pos = file_pos;
                    reader_redraw = true;
                }
            } else {
                if (file_pos < size) {
                    file_start_pos = file_next_pos;
                    for (int i = 0; i < WS_DISPLAY_WIDTH_TILES; i++) {
                        bitmap_vscroll_row(&ui_bitmap, i, 8 + font_height, 8, 128 - font_height);
                        bitmap_rect_fill(&ui_bitmap, i << 3, 8 + 128 - font_height, 8, font_height, BITMAP_COLOR_2BPP(2));
                    }
                }
            }
        }
    }

    return 0;
}
