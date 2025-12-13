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

#ifndef UI_FILEOPS_H__
#define UI_FILEOPS_H__

#include <stdint.h>
#include <wonderful.h>

/**
 * @brief Check if a given file name has the specified extension.
 * 
 * @param filename The file name to check.
 * @param ext The extension to check for.
 */
bool fileops_has_extension(const char __far *filename, const char __far *ext);

/**
 * @brief Check if a given file name points to a cartridge image file.
 */
bool fileops_is_rom(const char __far *filename);

/**
 * @brief Check if a given file name points to a cartridge image or flash file.
 */
bool fileops_has_rom_contents(const char __far *filename);

/**
 * @brief Delete the file and associated save data.
 * 
 * @return int16_t Error code
 */
int16_t fileops_delete_file_and_savedata(const char __far *filename);

/**
 * @brief Check if a file exists; offer the user an option to overwrite it, at which point the file and associated save data is deleted.
 * 
 * @return int16_t FR_OK on success, FR_EXIST if file exists (user rejects overwrite), other on error
 */
int16_t ui_fileops_check_file_overwrite(const char __far *filename);

/**
 * @brief Perform user-requested file deletion operation.
 */
int16_t ui_fileops_check_file_delete_by_user(const char __far *filename);

#endif /* UI_FILEOPS_H__ */
