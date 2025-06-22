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

#include <string.h>
#include "ui_osk.h"
#include "lang.h"
#include "main.h"
#include <wsx/utf8.h>
#include "util/input.h"

#define CALC_OSK_DIMENSIONS \
    uint8_t glyph_width = 16; \
    uint8_t glyph_height = 16; \
    uint8_t buffer_key_pad_height = 8; \
    \
    uint8_t osk_width = state->width * glyph_width; \
    uint8_t osk_height = (state->height + 1) * glyph_height + buffer_key_pad_height; \
    uint8_t osk_kheight = (state->height) * glyph_height; \
    uint8_t osk_x = ((WS_DISPLAY_WIDTH_PIXELS - osk_width) >> 1) & ~7; \
    uint8_t osk_by = ((WS_DISPLAY_HEIGHT_PIXELS - osk_height) >> 1) & ~7; \
    uint8_t osk_ky = osk_by + buffer_key_pad_height + glyph_height

static const char __far* get_osk_string(uint8_t tab) {
    switch (tab) {
        default:
        case 0: return lang_keys[LK_OSK_ALPHA_SMALL];
        case 1: return lang_keys[LK_OSK_ALPHA_LARGE];
    }
}

static const char __far* get_osk_fr_string(uint8_t fr) {
    switch (fr) {
        default:
        case UI_OSK_FR_ALPHA: return lang_keys[LK_OSK_ALPHA];
        case UI_OSK_FR_OK: return lang_keys[LK_OSK_OK];
    }
}

static uint16_t get_osk_fr_width(uint8_t fr) {
    uint16_t w = bitmapfont_get_string_width(get_osk_fr_string(fr), 65535);
    return ((w + 4) + 15) & ~15;
}

static int get_osk_fr_current(ui_osk_state_t *state) {
    for (int i = 0; i < UI_OSK_FR_COUNT; i++)
        if (state->x == state->fr_xpos[i])
            return i;
    return 0;
}

static int16_t get_button_width(ui_osk_state_t *state) {
    if (state->y == state->height - 1)
        return get_osk_fr_width(get_osk_fr_current(state)) / 16;
    return 1;
}

static void clear_osk_xy(ui_osk_state_t *state) {
    CALC_OSK_DIMENSIONS;

    ws_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(0),
        (osk_x + state->x * glyph_width) >> 3,
        (osk_ky + state->y * glyph_height) >> 3,
        get_button_width(state) * (glyph_width >> 3),
        glyph_height >> 3);
}

static void set_osk_xy(ui_osk_state_t *state, int8_t x, int8_t y, bool after_move) {
    CALC_OSK_DIMENSIONS;

    if (after_move) {
        if (y < 0) y = state->height - 1;
        else if (y >= state->height) y = 0;

        if (y == state->height - 1) {
            for (int i = 0; i < UI_OSK_FR_COUNT; i++) {
                if (x + 1 == state->fr_xpos[i]) {
                    if (i == 0)
                        x = state->fr_xpos[UI_OSK_FR_COUNT - 1];
                    else
                        x = state->fr_xpos[i - 1];
                    break;
                } else if (x - 1 == state->fr_xpos[i]) {
                    if ((i + 1) == UI_OSK_FR_COUNT)
                        x = state->fr_xpos[0];
                    else
                        x = state->fr_xpos[i + 1];
                    break;
                }
            }
        }

        if (x < 0) x = state->row_width[y] - 1;
        else if (x >= state->row_width[state->y]) x = 0;
    } else {
        if (y >= state->height) y = state->height - 1;
        else if (y < 0) y = 0;
    }

    if (y == state->height - 1) {
        for (int i = 0; i <= UI_OSK_FR_COUNT; i++) {
            if (i == UI_OSK_FR_COUNT) {
                x = state->fr_xpos[UI_OSK_FR_COUNT - 1];
            } else if (x <= state->fr_xpos[i]) {
                x = state->fr_xpos[i];
                break;
            }
        }
    }

    if (x >= state->row_width[y]) x = state->row_width[y] - 1;
    else if (x < 0) x = 0;

    state->x = x;
    state->y = y;

    ws_screen_modify_tiles(bitmap_screen2,
        ~WS_SCREEN_ATTR_PALETTE(0xF),
        WS_SCREEN_ATTR_PALETTE(1),
        (osk_x + state->x * glyph_width) >> 3,
        (osk_ky + state->y * glyph_height) >> 3,
        get_button_width(state) * (glyph_width >> 3),
        glyph_height >> 3);
}

static void fetch_osk_dimensions(ui_osk_state_t *state) {
    CALC_OSK_DIMENSIONS;

    const char __far* s = get_osk_string(state->tab);

    uint32_t ch;
    uint8_t curr_x = 0;
    uint8_t width = 1;
    uint8_t height = 1;
    while ((ch = wsx_utf8_decode_next(&s)) != 0) {
        if (ch == '\n') {
            state->row_width[height - 1] = curr_x;
            if (width < curr_x) width = curr_x;

            curr_x = 0;
            height++;
        } else {
            curr_x++;
        }
    }

    state->row_width[height - 1] = curr_x;
    if (width < curr_x) width = curr_x;

    // allocate final button row
    state->row_width[height] = width;
    height++;

    state->width = width;
    state->height = height;

    // calculate button widths and x positions
    curr_x = 0;
    for (int i = 0; i < UI_OSK_FR_COUNT; i++) {
        uint8_t w = get_osk_fr_width(i) / glyph_width;
        if (i == UI_OSK_FR_COUNT - 1)
            curr_x = width - w;
        state->fr_xpos[i] = curr_x;
        curr_x += w;
    }
}

static uint32_t osk_get_char_at(ui_osk_state_t *state, uint16_t x, uint16_t y) {
    const char __far* s = get_osk_string(state->tab);

    uint32_t ch;
    uint16_t curr_x = 0;
    uint16_t curr_y = 0;
    while ((ch = wsx_utf8_decode_next(&s)) != 0) {
        if (curr_x == x && curr_y == y) {
            if (ch == 0x2423) // open box
                return ' ';
            return ch;
        }

        if (ch == '\n') {
            curr_x = 0;
            curr_y++;
        } else {
            curr_x++;
        }
    }

    return 0;
}

static void redraw_osk_buffer_text(ui_osk_state_t *state, bool clear) {
    CALC_OSK_DIMENSIONS;

    if (clear)
        bitmap_rect_fill(&ui_bitmap, 0, osk_by, WS_DISPLAY_WIDTH_PIXELS, glyph_height, BITMAP_COLOR(2, 3, BITMAP_COLOR_MODE_STORE));

    bitmapfont_draw_string(&ui_bitmap, 4, osk_by, state->buffer, WS_DISPLAY_WIDTH_PIXELS - 8);
}

static void redraw_osk(ui_osk_state_t *state) {
    CALC_OSK_DIMENSIONS;

    ui_layout_clear_bars_content();
    bitmapfont_set_active_font(font16_bitmap);

    // draw outline
    bitmap_rect_draw(&ui_bitmap, osk_x - 1, osk_ky - 1, osk_width + 2, osk_kheight + 2,
        BITMAP_COLOR(3, 3, BITMAP_COLOR_MODE_STORE), true);

    redraw_osk_buffer_text(state, false);

    // draw key area
    const char __far* s = get_osk_string(state->tab);

    uint32_t ch;
    uint8_t osk_cx = osk_x;
    uint8_t osk_cy = osk_ky;
    while ((ch = wsx_utf8_decode_next(&s)) != 0) {
        if (ch == '\n') {
            osk_cx = osk_x;
            osk_cy += glyph_height;
        } else {
            uint8_t width = bitmapfont_get_char_width(ch);
            bitmapfont_draw_char(&ui_bitmap, osk_cx + ((glyph_width - width) >> 1), osk_cy + 1, ch);
            osk_cx += glyph_width;
        }
    }

    osk_cy += glyph_height;
    for (int i = 0; i < UI_OSK_FR_COUNT; i++) {
        s = get_osk_fr_string(i);
        uint16_t w = bitmapfont_get_string_width(s, 65535);
        uint16_t fw = ((w + 4) + 15) & ~15;
        bitmapfont_draw_string(&ui_bitmap,
            osk_x + glyph_width * state->fr_xpos[i] + ((fw - w) >> 1),
            osk_cy + 1, s, fw);
    }
}

static void osk_insert_char(ui_osk_state_t *state, uint32_t ch) {
    if (ch == 0) return;
    if ((state->buflen - state->bufpos) <= 4) return;

    char* s = state->buffer + state->bufpos;

    s = wsx_utf8_encode_next(s, ch);
    *s = 0;

    state->bufpos = s - state->buffer;
}

static void osk_delete_char(ui_osk_state_t *state) {
    char __far* s = state->buffer;
    char __far* sp = NULL;
    char __far* spp = NULL;

    while (true) {
        spp = sp;
        sp = s;
        if (!wsx_utf8_decode_next(&s)) break;
    }

    if (spp != NULL) {
        *spp = 0;
        state->bufpos = spp - state->buffer;
    }
}

void ui_osk(ui_osk_state_t *state) {
    state->bufpos = strlen(state->buffer);
    state->tab = 0;
    state->x = 0;

osk_full_redraw:
    fetch_osk_dimensions(state);
    redraw_osk(state);
    set_osk_xy(state, state->x, state->y, false);

    while (true) {
        wait_for_vblank();
        input_update();

        int8_t nx = state->x;
        int8_t ny = state->y;

        if (input_pressed & WS_KEY_A) {
            if (state->y == state->height - 1) {
                switch (get_osk_fr_current(state)) {
                    case UI_OSK_FR_ALPHA:
                        state->tab = 1 - state->tab;
                        goto osk_full_redraw;
                    case UI_OSK_FR_OK:
                        return;
                }
            } else {
                osk_insert_char(state, osk_get_char_at(state, state->x, state->y));
                redraw_osk_buffer_text(state, true);
            }
        }
        if (input_pressed & WS_KEY_B) {
            osk_delete_char(state);
            redraw_osk_buffer_text(state, true);
        }

        if (input_pressed & KEY_UP) ny--;
        if (input_pressed & KEY_DOWN) ny++;
        if (input_pressed & KEY_LEFT) nx--;
        if (input_pressed & KEY_RIGHT) nx++;

        if (nx != state->x || ny != state->y) {
            clear_osk_xy(state);
            set_osk_xy(state, nx, ny, true);
        }
    }
}
