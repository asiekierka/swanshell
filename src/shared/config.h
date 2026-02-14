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

#ifndef CONFIG_H_
#define CONFIG_H_

// #define CONFIG_DEBUG_FORCE_MONO
// #define CONFIG_DEBUG_FORCE_DISABLE_SECTOR_BUFFER
#define CONFIG_ENABLE_WALLPAPER

#define CONFIG_MEMLAYOUT_SECTOR_BUFFER_SIZE 8192
#define CONFIG_MEMLAYOUT_STACK_BUFFER_SIZE 64

#define CONFIG_FILESELECT_PATH_MEMORY_DEPTH 12

#define CONFIG_FONT_BITMAP_SHIFT 8
#define CONFIG_FONT_CHAR_GAP 0

#endif /* CONFIG_H_ */
