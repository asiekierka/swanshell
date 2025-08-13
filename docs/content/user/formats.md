---
title: 'Supported formats'
weight: 50
---

swanshell currently supports the following file formats:

| Format | Description |
| ------ | ----------- |
| .ws, .wsc, .pc2 | Cartridge images |
| .bfb | [BootFriend](https://wiki.asie.pl/doku.php?id=project:homebrew:wsbootfriend) programs (testing only) |
| .bmp | Bitmap images (up to 16 colors, 224x144) |
| .vgm | Music files (see below) |
| .wav | Music files (see below) |

### Cartridge save data

The cartridge save data follows the ares emulator's naming scheme:

- `.ram` - SRAM save data,
- `.eeprom` - EEPROM save data,
- `.flash` - NOR flash cartridge data.

For NOR flash cartridges (WW), if a `.flash` file exists, its contents are preferred to the `.ws`/`.wsc` file.

### .vgm support limitations

Only one chip at a time is supported, from the following list:

| Chip | Notes |
| ---- | ----- |
| DMG (GB/GBC) | |
| PSG (SN76489) | |
| WonderSwan | |

### .wav support limitations

Recommended (higher quality, lower CPU usage):

- Sample rate: 4000, 6000, 12000, 24000 Hz
- Channel layout: Mono
- Encoding: Unsigned 8-bit

Supported:

- Sample rate: 1 - 65535 Hz
- Channel layout: Mono, stereo
- Encoding: Unsigned 8-bit, signed 16-bit
