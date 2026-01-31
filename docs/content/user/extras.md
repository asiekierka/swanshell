---
title: 'Additional features'
weight: 60
---

## Gamepad mode

The *gamepad mode* allows using your console as an USB HID gamepad.

Press `START`, then select `Tools` -> `Gamepad mode` to activate.

Note that, due to the console's non-standard button layout, only games and programs which support custom controller button bindings should be expected to work correctly.

## Additional supported formats

| Format | Description |
| ------ | ----------- |
| .bfb | [BootFriend](https://wiki.asie.pl/doku.php?id=project:homebrew:wsbootfriend) programs[^bfb] |
| .bmp | Bitmap images (up to 16 colors, 224x144) |
| .vgm, .vgz | Music files[^vgm] |
| .wav | Music files[^wav] |

[^bfb]: Primarily meant for development use.
[^vgm]: DMG (GB/GBC), PSG (SN76489), and WS/WSC music files supported.
[^wav]: It is recommended to make your WAV files 4000, 6000, 12000 or 24000 Hz, 8-bit, mono.

## Loading programs via USB

The `Tools` -> `Launch via XMODEM` option allows loading programs via nileswan's USB port. `.ws`/`.wsc` and `.bfb` programs can be loaded this way.

The protocol used is XMODEM. On Windows, you can use [Tera Term](https://teratermproject.github.io/index-en.html) to send files via XMODEM; on Linux, `minicom` and `lrzsz` are viable solutions.
