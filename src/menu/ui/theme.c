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

#include <nilefs/ff.h>
#include <ws.h>
#include "theme.h"
#include "errors.h"
#include "settings.h"
#include "strings.h"
#include "ui/ui.h"
#include "util/asset_heap.h"
#include "util/bmp.h"
#include "util/file.h"

static uint8_t theme_bank = 0xFF;

uint8_t theme_get_bank(void) {
    return theme_bank;
}

bool theme_has_feature(uint16_t feature) {
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        theme_header_t __far* header = THEME_MEM_HEADER;
        return (header->features & feature) == feature;
    });
}

int theme_add_wallpaper(uint16_t feature, const char __far *path) {
    uint16_t palette[16] = {0};
    char path_copy[FF_LFN_BUF + 1];
    path_copy[FF_LFN_BUF] = 0;
    strncpy(path_copy, path, FF_LFN_BUF);

    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, 0, {
        bmp_header_t __far* bmp = MK_FP(0x1000, 0x0000);
        int result = f_read_unbanked(bmp, path_copy, 65535, NULL);
        if (result != FR_OK) {
            return result;
        }

        if (!bmp_is_header_valid(bmp) || bmp->width != WS_DISPLAY_WIDTH_PIXELS
            || bmp->height > WS_DISPLAY_HEIGHT_PIXELS || bmp->compression != 0) {
            return ERR_FILE_FORMAT_INVALID;
        }

        uint8_t __far *data = MK_FP(0x1000, bmp->data_start);
        int color_count = bmp_calculate_color_count(bmp, data);
        uint16_t pitch = bmp_calculate_pitch_bytes(bmp);

        uint16_t bmp_loader_flags = BMP_LOADER_FLAG_4BPP;
        void *bmp_loader_dest = (void*) 0x8000;
        if (color_count > 16) {
            return ERR_FILE_FORMAT_INVALID;
        }

        ui_unload_wallpaper();
        uint8_t __far *bmp_palette = MK_FP(0x1000, 14 + bmp->header_size);
        bmp_convert_palette_to_rgb4(palette, bmp_palette, color_count, bmp_loader_flags);
        result = bmp_convert_data(bmp->width, bmp->height, pitch, bmp->bpp, bmp_loader_dest, data, bmp_loader_flags);
        if (result) {
            return result;
        }

        uint16_t height = bmp->height;
        ws_bank_with_ram(theme_bank, {
            theme_header_t __far* header = THEME_MEM_HEADER;
            uint8_t __far *dest = MK_FP(0x1000, 512);

            if (feature == THEME_FEATURE_WALLPAPER_SHARED) {
                header->features |= THEME_FEATURE_WALLPAPER_HORIZONTAL | THEME_FEATURE_WALLPAPER_VERTICAL | THEME_FEATURE_WALLPAPER_SHARED;
            } else {
                header->features &= ~THEME_FEATURE_WALLPAPER_SHARED;
                if (!(header->features & THEME_FEATURE_WALLPAPER_HORIZONTAL)
                    && (header->features & THEME_FEATURE_WALLPAPER_VERTICAL)
                    && feature == THEME_FEATURE_WALLPAPER_HORIZONTAL) {
                    // adding distinct horizontal wallpaper to vertical wallpaper, move vertical data down
                    memcpy(MK_FP(0x1000, 512 + 16384), MK_FP(0x1000, 512), 16384);
                }
                header->features |= feature;
                dest = theme_get_pointer(feature);
            }

            memcpy(dest, palette, 32);
            if (height == WS_DISPLAY_HEIGHT_PIXELS) {
                memcpy(dest + 32, (uint16_t*) 0x8000, 16128);
            } else {
                memset(dest + 32, 0, 16128);

                uint16_t copy_offset = ((WS_DISPLAY_HEIGHT_PIXELS - height) >> 1) * 4;
                uint16_t copy_height = height * 4;
                uint8_t *ptr = (uint8_t*) 0x8000;

                for (int i = 0; i < WS_SCREEN_WIDTH_TILES; i++, ptr += (WS_DISPLAY_HEIGHT_PIXELS * 4)) {
                    memcpy(dest + 32 + (WS_DISPLAY_HEIGHT_PIXELS * 4 * i) + copy_offset, ptr, copy_height);
                }
            }
        });
    });

    theme_save();
    return 0;
}

void theme_remove_wallpaper(uint16_t feature) {
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        theme_header_t __far* header = THEME_MEM_HEADER;
        if (feature == THEME_FEATURE_WALLPAPER_SHARED) {
            // Remove all wallpaper data
            header->features &= ~(THEME_FEATURE_WALLPAPER_HORIZONTAL | THEME_FEATURE_WALLPAPER_VERTICAL | THEME_FEATURE_WALLPAPER_SHARED);
        } else {
            header->features &= ~(feature | THEME_FEATURE_WALLPAPER_SHARED);
            if (!(header->features & THEME_FEATURE_WALLPAPER_SHARED) && (header->features & THEME_FEATURE_WALLPAPER_VERTICAL) && feature == THEME_FEATURE_WALLPAPER_HORIZONTAL) {
                // removed horizontal, wallpapers were not shared, move vertical data up
                memcpy(MK_FP(0x1000, 512), MK_FP(0x1000, 512 + 16384), 16384);
            }
        }
    });

    theme_save();
}

void __far *theme_get_pointer(uint16_t feature) {
    if (!theme_initialized()) return NULL;

    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        theme_header_t __far* header = THEME_MEM_HEADER;
        uint16_t size = 512;
        if (header->features & THEME_FEATURE_WALLPAPER_HORIZONTAL) {
            if (feature == THEME_FEATURE_WALLPAPER_HORIZONTAL) return MK_FP(0x1000, size);
            // catch VERTICAL + SHARED
            if ((header->features & 6) != 6) {
                size += 16384;
            }
        }
        if (header->features & THEME_FEATURE_WALLPAPER_VERTICAL) {
            if (feature == THEME_FEATURE_WALLPAPER_VERTICAL) return MK_FP(0x1000, size);
            size += 16384;
        }
        return NULL;
    });
}

uint16_t theme_calculate_filesize(void) {
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        theme_header_t __far* header = THEME_MEM_HEADER;
        uint16_t size = 512;
        if (header->features & THEME_FEATURE_WALLPAPER_HORIZONTAL) size += 16384;
        if (header->features & THEME_FEATURE_WALLPAPER_VERTICAL) size += 16384;
        return size;
    });
}

void theme_reset(void) {
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        theme_header_t __far* header = THEME_MEM_HEADER;
        memset(header, 0, sizeof(theme_header_t));
        header->magic = THEME_MAGIC;
        header->version = THEME_VERSION;
        header->features = 0;
        header->accent_color = SETTING_THEME_ACCENT_COLOR_DEFAULT;
    });
}

void theme_to_iram(void) {
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        theme_header_t __far* header = THEME_MEM_HEADER;
        // TODO
        // settings.accent_color = header->accent_color;
    });
}

__attribute__((noinline))
static int _theme_load(void) {
    theme_header_t __far* header = THEME_MEM_HEADER;
    // Clear magic to catch failed reads
    header->magic = 0;
    return f_read_unbanked(header, s_path_theme_dat, 65535, NULL);
}

int theme_init(void) {
    theme_bank = asset_heap_alloc_banks(1);
    ws_bank_with_flash_ram(WS_CART_BANK_FLASH_ENABLE, theme_bank, {
        _theme_load();

        theme_header_t __far* header = THEME_MEM_HEADER;
        if (header->magic != THEME_MAGIC || header->version > THEME_VERSION) {
            theme_reset();
        }
    });
    theme_to_iram();
    return 0;
}

int theme_save(void) {
    if (!theme_initialized()) return FR_OK;
    uint16_t size = theme_calculate_filesize();

    FIL fp;
    int16_t result = f_open_far(&fp, s_path_theme_dat, FA_WRITE | FA_OPEN_ALWAYS);
    uint16_t bw;
    if (result) return result;

    ws_bank_with_rom0(theme_bank, {
        result = f_write(&fp, MK_FP(0x2000, 0x0000), size, &bw);
        f_close(&fp);
        return result;
    });
}
