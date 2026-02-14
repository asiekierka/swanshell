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

#include <stdio.h>
#include <string.h>
#include <ws/display.h>
#include <ws/system.h>

#include "lang_gen.h"
#include "settings.h"
#include "lang.h"
#include "strings.h"
#include "ui/ui.h"
#include "ui/ui_about.h"
#include "ui/ui_dialog.h"
#include "ui/ui_rtc_clock.h"

DEFINE_STRING_LOCAL(s_file_show_hidden_key, "FileShowHidden");
DEFINE_STRING_LOCAL(s_file_sort_order_key, "FileSortOrder");
DEFINE_STRING_LOCAL(s_file_view_key, "FileView");
DEFINE_STRING_LOCAL(s_file_hide_icons_key, "FileHideIcons");
DEFINE_STRING_LOCAL(s_program_fast_sram_key, "ProgFastSRAM");
DEFINE_STRING_LOCAL(s_program_fx_bios_key, "ProgFxCmptBios");
DEFINE_STRING_LOCAL(s_cart_mcu_spi_speed_key, "CartMcuSpiSpeed");
DEFINE_STRING_LOCAL(s_language, "Language");
DEFINE_STRING_LOCAL(s_theme_accent_color_key, "ThemeAccentColor");
DEFINE_STRING_LOCAL(s_theme_dark_mode_key, "ThemeDarkMode");
DEFINE_STRING_LOCAL(s_joy_repeat_first, "JoyRepeatFirst");
DEFINE_STRING_LOCAL(s_joy_repeat_next, "JoyRepeatNext");

settings_t settings;

static const setting_t __far setting_file_show_hidden = {
    s_file_show_hidden_key,
    LK_SETTINGS_FILE_SHOW_HIDDEN_KEY,
    LK_SETTINGS_FILE_SHOW_HIDDEN_HELP,
    SETTING_TYPE_FLAG,
    0,
    .flag = {
        &settings.file_flags,
        SETTING_FILE_SHOW_HIDDEN_SHIFT,
        LK_NO, LK_YES
    }
};

static const setting_t __far setting_file_hide_icons = {
    s_file_show_hidden_key,
    LK_SETTINGS_FILE_HIDE_ICONS_KEY,
    0,
    SETTING_TYPE_FLAG,
    0,
    .flag = {
        &settings.file_flags,
        SETTING_FILE_HIDE_ICONS_SHIFT,
        LK_NO, LK_YES
    }
};

static const uint16_t __wf_rom settings_file_sort_order_name_table[] = {
    LK_SETTINGS_FILE_SORT_DEFAULT,
    LK_SETTINGS_FILE_SORT_NAME_ASC,
    LK_SETTINGS_FILE_SORT_NAME_DESC,
    LK_SETTINGS_FILE_SORT_DATE_ASC,
    LK_SETTINGS_FILE_SORT_DATE_DESC,
    LK_SETTINGS_FILE_SORT_SIZE_ASC,
    LK_SETTINGS_FILE_SORT_SIZE_DESC
};

static void settings_file_sort_order_name(uint16_t value, char *buf, int buf_len) {
    strncpy(buf, lang_keys[settings_file_sort_order_name_table[value % 7]], buf_len);
}

static const setting_t __far setting_file_sort_order = {
    s_file_sort_order_key,
    LK_SETTINGS_FILE_SORT_KEY,
    LK_SETTINGS_FILE_SORT_HELP,
    SETTING_TYPE_CHOICE_BYTE,
    0,
    NULL,
    .choice = {
        &settings.file_sort,
        0, 6,
        NULL,
        settings_file_sort_order_name
    }
};

static const uint16_t __wf_rom settings_file_view_name_table[] = {
    LK_SETTINGS_FILE_VIEW_LARGE,
    LK_SETTINGS_FILE_VIEW_SMALL
};

static void settings_file_view_name(uint16_t value, char *buf, int buf_len) {
    strncpy(buf, lang_keys[settings_file_view_name_table[value & 1]], buf_len);
}

static const setting_t __far setting_file_view = {
    s_file_view_key,
    LK_SETTINGS_FILE_VIEW_KEY,
    LK_SETTINGS_FILE_VIEW_HELP,
    SETTING_TYPE_CHOICE_BYTE,
    0,
    NULL,
    .choice = {
        &settings.file_view,
        0, 1,
        NULL,
        settings_file_view_name
    }
};

static const setting_category_t __far settings_file = {
    LK_SETTINGS_FILE_KEY,
    0,
    &settings_root,
    4,
    {
        &setting_file_view,
        &setting_file_sort_order,
        &setting_file_show_hidden,
        &setting_file_hide_icons
    }
};

static const setting_t __far setting_file = {
    NULL,
    LK_SETTINGS_FILE_KEY,
    0,
    SETTING_TYPE_CATEGORY,
    0,
    NULL,
    .category = { &settings_file }
};

#define LANGUAGE_COUNT 6

static const uint16_t __wf_rom settings_language_name_table[] = {
    LK_LANG_EN,
    LK_LANG_PL,
    LK_LANG_DE,
    LK_LANG_FR,
    LK_LANG_ES,
    LK_LANG_ZH_HANS
};

static const void __far* __far settings_language_table[] = {
    lang_keys_en,
    lang_keys_pl,
    lang_keys_de,
    lang_keys_fr,
    lang_keys_es,
    lang_keys_zh_Hans
};

void settings_language_update(void) {
    lang_keys = settings_language_table[settings.language];
}

static void settings_language_on_change(const struct setting *set) {
    settings_language_update();
}

static void settings_language_name(uint16_t value, char *buf, int buf_len) {
    strncpy(buf, lang_keys[settings_language_name_table[value % LANGUAGE_COUNT]], buf_len);
}

const setting_t __far setting_language = {
    s_language,
    LK_SETTINGS_LANGUAGE_KEY,
    0,
    SETTING_TYPE_CHOICE_BYTE,
    SETTING_FLAG_CHOICE_LIST,
    settings_language_on_change,
    .choice = {
        &settings.language,
        0, LANGUAGE_COUNT - 1,
        NULL,
        settings_language_name
    }
};

static const setting_t __far setting_program_fast_sram = {
    s_program_fast_sram_key,
    LK_SETTINGS_PROG_SRAM_OVERCLOCK,
    LK_SETTINGS_PROG_SRAM_OVERCLOCK_HELP,
    SETTING_TYPE_FLAG,
    0,
    .flag = {
        &settings.prog.flags,
        SETTING_PROG_FLAG_SRAM_OVERCLOCK_SHIFT,
        LK_NO, LK_YES
    }
};

static const setting_t __far setting_program_fx_bios = {
    s_program_fx_bios_key,
    LK_SETTINGS_PROG_FX_BIOS,
    LK_SETTINGS_PROG_FX_BIOS_HELP,
    SETTING_TYPE_FLAG,
    0,
    .flag = {
        &settings.prog.flags,
        SETTING_PROG_FLAG_FX_COMPATIBLE_BIOS_SHIFT,
        LK_ATHENABIOS_SUFFIX_NATIVE, LK_ATHENABIOS_SUFFIX_COMPATIBLE
    }
};

static const setting_category_t __far settings_program = {
    LK_SETTINGS_PROG_KEY,
    0,
    &settings_root,
    2,
    {
        &setting_program_fast_sram,
        &setting_program_fx_bios
    }
};

static const setting_t __far setting_program = {
    NULL,
    LK_SETTINGS_PROG_KEY,
    0,
    SETTING_TYPE_CATEGORY,
    0,
    NULL,
    .category = { &settings_program }
};

static const uint16_t __wf_rom settings_spi_speed_table[] = {
    LK_SETTINGS_CART_MCU_SPI_SPEED_384KHZ,
    LK_SETTINGS_CART_MCU_SPI_SPEED_6MHZ,
    LK_SETTINGS_CART_MCU_SPI_SPEED_24MHZ
};

static void settings_spi_speed_name(uint16_t value, char *buf, int buf_len) {
    strncpy(buf, lang_keys[settings_spi_speed_table[value]], buf_len);
}

static const setting_t __far setting_cart_mcu_spi_speed = {
    s_cart_mcu_spi_speed_key,
    LK_SETTINGS_CART_MCU_SPI_SPEED,
    LK_SETTINGS_CART_MCU_SPI_SPEED_HELP,
    SETTING_TYPE_CHOICE_BYTE,
    0,
    .choice = {
        &settings.mcu_spi_speed,
        0, SETTING_MCU_SPI_SPEED_COUNT - 1,
        NULL,
        settings_spi_speed_name
    }
};

static void setting_cart_set_rtc_time_action(const struct setting *set) {
    int16_t result = ui_rtc_clock();
    if (result != 0) {
        ui_layout_bars();
        ui_dialog_error_check(result, lang_keys[LK_SETTINGS_CART_SET_RTC_TIME], 0);
    }
}

static const setting_t __far setting_cart_set_rtc_time = {
    NULL,
    LK_SETTINGS_CART_SET_RTC_TIME,
    NULL,
    SETTING_TYPE_ACTION,
    0,
    setting_cart_set_rtc_time_action
};

static const setting_category_t __far settings_cart = {
    LK_SETTINGS_CART_KEY,
    0,
    &settings_root,
    2,
    {
        &setting_cart_mcu_spi_speed,
        &setting_cart_set_rtc_time
    }
};

static const setting_t __far setting_cart = {
    NULL,
    LK_SETTINGS_CART_KEY,
    0,
    SETTING_TYPE_CATEGORY,
    0,
    NULL,
    .category = { &settings_cart }
};

static void settings_theme_accent_color_on_change(const struct setting *set) {
    bool dark = settings.accent_color_high & SETTING_THEME_DARK_MODE;
    int shade = ui_rgb_to_shade(settings.accent_color);
    bool dark_titlebar_text = shade <= (dark ? 5 : 1);
    if (ws_system_is_color_active()) {
        WS_DISPLAY_COLOR_MEM(2)[2] = settings.accent_color;
        WS_DISPLAY_COLOR_MEM(2)[3] = dark_titlebar_text ? 0x000 : 0xFFF;
    }
    outportb(WS_SCR_PAL_2_PORT + 1, shade | (dark_titlebar_text ? 0x70 : 0x00));
}

static void settings_theme_dark_mode_on_change(const struct setting *set) {
    bool dark = settings.accent_color_high & SETTING_THEME_DARK_MODE;
    if (ws_system_is_color_active()) {
        if (!ui_has_wallpaper())
            WS_DISPLAY_COLOR_MEM(0)[0] = dark ? 0x000 : 0xFFF;
        WS_DISPLAY_COLOR_MEM(0)[2] = dark ? 0x000 : 0xFFF;
        WS_DISPLAY_COLOR_MEM(0)[3] = dark ? 0xFFF : 0x000;
        WS_DISPLAY_COLOR_MEM(1)[3] = dark ? 0x000 : 0xFFF;
        WS_DISPLAY_COLOR_MEM(1)[2] = dark ? 0xFFF : 0x000;
    }
    outportb(WS_SCR_PAL_0_PORT + 1, dark ? 0x07 : 0x70);
    outportb(WS_SCR_PAL_1_PORT + 1, dark ? 0x70 : 0x07);
    outportb(WS_SCR_PAL_2_PORT, dark ? 0x07 : 0x70);
    settings_theme_accent_color_on_change(set);
}

static const setting_t __far setting_theme_dark_mode = {
    s_theme_dark_mode_key,
    LK_SETTINGS_THEME_DARK_MODE,
    0,
    SETTING_TYPE_FLAG,
    0,
    settings_theme_dark_mode_on_change,
    .flag = {
        &settings.accent_color_high,
        SETTING_THEME_DARK_MODE_SHIFT,
        LK_SETTINGS_THEME_LIGHT, LK_SETTINGS_THEME_DARK
    }
};

static const setting_t __far setting_theme_accent_color = {
    s_theme_accent_color_key,
    LK_SETTINGS_THEME_ACCENT_COLOR,
    0,
    SETTING_TYPE_COLOR,
    0,
    settings_theme_accent_color_on_change,
    .color = { &settings.accent_color }
};

static const setting_category_t __far settings_theme = {
    LK_SETTINGS_THEME_KEY,
    0,
    &settings_root,
    2,
    {
        &setting_theme_dark_mode,
        &setting_theme_accent_color
    }
};

static const setting_t __far setting_theme = {
    NULL,
    LK_SETTINGS_THEME_KEY,
    0,
    SETTING_TYPE_CATEGORY,
    0,
    NULL,
    .category = { &settings_theme }
};

// TODO: Implement friendlier configuration method for repeat delay/rate
static void settings_ticks_name(uint16_t value, char *buf, int buf_len) {
    uint16_t value_ms = ((value * 53) >> 2);
    value_ms = ((value_ms + 4) / 5) * 5;

    snprintf(buf, buf_len, s_milliseconds_tpl, value_ms);
}

static const setting_t __far setting_repeat_delay = {
    s_joy_repeat_first,
    LK_SETTINGS_CONTROLS_REPEAT_DELAY,
    0,
    SETTING_TYPE_CHOICE_BYTE,
    SETTING_FLAG_CHOICE_LIST,
    NULL,
    .choice = {
        &settings.joy_repeat_first_ticks,
        9, 37,
        NULL,
        settings_ticks_name
    }
};

static const setting_t __far setting_repeat_next_delay = {
    s_joy_repeat_next,
    LK_SETTINGS_CONTROLS_REPEAT_RATE,
    0,
    SETTING_TYPE_CHOICE_BYTE,
    SETTING_FLAG_CHOICE_LIST,
    NULL,
    .choice = {
        &settings.joy_repeat_next_ticks,
        5, 36,
        NULL,
        settings_ticks_name
    }
};

static const setting_category_t __far settings_controls = {
    LK_SETTINGS_CONTROLS_KEY,
    0,
    &settings_root,
    2,
    {
        &setting_repeat_delay,
        &setting_repeat_next_delay
    }
};

static const setting_t __far setting_controls = {
    NULL,
    LK_SETTINGS_CONTROLS_KEY,
    0,
    SETTING_TYPE_CATEGORY,
    0,
    NULL,
    .category = { &settings_controls }
};

static void setting_about_action(const struct setting *set) {
    ui_about();
    ui_layout_bars();
}

static const setting_t __far setting_about = {
    NULL,
    LK_SETTINGS_ABOUT,
    NULL,
    SETTING_TYPE_ACTION,
    SETTING_FLAG_ACTION_NO_ARROW,
    setting_about_action
};

const setting_category_t __far settings_root = {
    LK_SETTINGS_KEY,
    0,
    NULL,
    7,
    {
        &setting_file,
        &setting_theme,
        &setting_controls,
        &setting_program,
        &setting_cart,
        &setting_language,
        &setting_about
    }
};
