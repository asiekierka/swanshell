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

#ifndef _STRINGS_H_
#define _STRINGS_H_

#define DEFINE_STRING_LOCAL(name, value) static const char __far name[] = value
#ifdef STRINGS_H_IMPLEMENTATION
#define DEFINE_STRING(name, value) const char __far name[] = value
#else
#define DEFINE_STRING(name, value) extern const char __far name[]
#endif

DEFINE_STRING(s_color, "#%03x");

DEFINE_STRING(s_path_sep, "/");
DEFINE_STRING(s_dot, ".");
DEFINE_STRING(s_dotdot, "..");

DEFINE_STRING(s_file_ext_bfb, ".bfb");
DEFINE_STRING(s_file_ext_bmp, ".bmp");
DEFINE_STRING(s_file_ext_bin, ".bin");
DEFINE_STRING(s_file_ext_com, ".com");
DEFINE_STRING(s_file_ext_pc2, ".pc2");
DEFINE_STRING(s_file_ext_raw, ".raw");
DEFINE_STRING(s_file_ext_vgm, ".vgm");
DEFINE_STRING(s_file_ext_vgz, ".vgz");
DEFINE_STRING(s_file_ext_wav, ".wav");
DEFINE_STRING(s_file_ext_ws, ".ws");
DEFINE_STRING(s_file_ext_wsc, ".wsc");

// Follow ares convention on save filenames.
DEFINE_STRING(s_file_ext_sram, ".ram");
DEFINE_STRING(s_file_ext_eeprom, ".eeprom");
DEFINE_STRING(s_file_ext_flash, ".flash");
DEFINE_STRING(s_file_ext_rtc, ".rtc");

DEFINE_STRING(s_path_save_ini, "/NILESWAN/SAVE.INI");
DEFINE_STRING(s_path_config_ini, "/NILESWAN/CONFIG.INI");

DEFINE_STRING(s_error_unknown, "Error %d");

DEFINE_STRING(s_path_fbin, "/NILESWAN/fbin");
DEFINE_STRING(s_path_athenabios_compatible, "/NILESWAN/BIOSATHC.RAW");
DEFINE_STRING(s_path_athenabios_native, "/NILESWAN/BIOSATHN.RAW");
DEFINE_STRING(s_bios, "bios");
DEFINE_STRING(s_freya, "freya");
DEFINE_STRING(s_athenabios_tpl, "AthenaBIOS ");
DEFINE_STRING(s_freyabios_tpl, "FreyaBIOS %c.%c.%s");
DEFINE_STRING(s_freyaos_tpl, "FreyaOS %c.%c.%s");

#endif /* _STRINGS_H_ */
