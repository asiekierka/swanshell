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

#include <stdlib.h>
#include <string.h>
#include <nile.h>
#include <nilefs.h>
#include "lang.h"
#include "settings.h"
#include "strings.h"
#include "util/ini.h"

void settings_reset(void) {
    memset(&settings, 0, sizeof(settings));
    lang_keys = lang_keys_en;
}

static int16_t settings_load_category(FIL *fp, const setting_category_t __far *cat, const char *key, const char *value) {
    for (int i = 0; i < cat->entry_count; i++) {
        const setting_t __far *s = cat->entries[i];
        int16_t result = FR_OK;

        if (s->type == SETTING_TYPE_CATEGORY) {
            result = settings_load_category(fp, s->category.value, key, value);
        } else if (s->key == NULL || strcasecmp(s->key, key)) {
            continue;
        } else if (s->type == SETTING_TYPE_FLAG) {
            int v = atoi(value);
            if (v) {
                *s->flag.value |= 1 << s->flag.bit;
            } else {
                *s->flag.value &= ~(1 << s->flag.bit);
            }
        } else if (s->type == SETTING_TYPE_CHOICE_BYTE) {
            uint16_t v = atoi(value);
            if (v <= s->choice.max && (!s->choice.allowed || s->choice.allowed(v))) {
                *((uint8_t*) s->choice.value) = v;
            }
        }

        if (result != FR_OK)
            return result;

        if (s->on_change)
            s->on_change(s);
    }

    return FR_OK;
}

int16_t settings_load(void) {
    FIL fp;
    int16_t result;
    char buffer[FF_LFN_BUF + 32];
    char *key, *value;
    ini_next_result_t ini_result;

    settings_reset();

    strcpy(buffer, s_path_config_ini);
    result = f_open(&fp, buffer, FA_OPEN_EXISTING | FA_READ);
    if (result != FR_OK)
        goto settings_load_error;

    while (true) {
        ini_result = ini_next(&fp, buffer, sizeof(buffer), &key, &value);
        if (ini_result == INI_NEXT_ERROR) {
            result = FR_INT_ERR;
            goto settings_load_error_opened;
        } else if (ini_result == INI_NEXT_FINISHED) {
            break;
        } else if (ini_result == INI_NEXT_KEY_VALUE) {
            settings_load_category(&fp, &settings_root, key, value);
        }
    }

settings_load_error_opened:
    result = result || f_close(&fp);

settings_load_error:
    if (result != FR_OK) {
        settings_save();
    }
    return result;
}

DEFINE_STRING_LOCAL(s_ini_entry_flag, "%s=%d\n");

static int16_t settings_save_category(FIL *fp, const setting_category_t __far *cat) {
    for (int i = 0; i < cat->entry_count; i++) {
        const setting_t __far *s = cat->entries[i];
        int16_t result = FR_OK;

        if (s->type == SETTING_TYPE_CATEGORY) {
            result = settings_save_category(fp, s->category.value);
        } else if (s->key == NULL) {
            continue;
        } else if (s->type == SETTING_TYPE_FLAG) {
            int v = (*s->flag.value & (1 << s->flag.bit)) ? 1 : 0;
            result = f_printf(fp, s_ini_entry_flag, s->key, v) < 0 ? FR_INT_ERR : FR_OK;
        } else if (s->type == SETTING_TYPE_CHOICE_BYTE) {
            result = f_printf(fp, s_ini_entry_flag, s->key, *((uint8_t*) s->choice.value)) < 0 ? FR_INT_ERR : FR_OK;
        }

        if (result != FR_OK)
            return result;
    }

    return FR_OK;
}

int16_t settings_save(void) {
    FIL fp;
    int16_t result;
    char tmp_buf[20];

    strcpy(tmp_buf, s_path_config_ini);
    result = f_open(&fp, tmp_buf, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK)
        return result;

    result = settings_save_category(&fp, &settings_root);
    if (result != FR_OK)
        return result;

    return f_close(&fp);
}
