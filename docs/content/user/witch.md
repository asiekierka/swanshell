---
title: 'WW support'
weight: 20
---

{{< hint type=important >}}
WonderWitch is a product of Qute Corporation. swanshell is not endorsed or supported by Qute Corporation.

Please **refrain** from contacting Qute Corporation with any inquiries regarding running WonderWitch software on nileswan.
{{< /hint >}}

swanshell has extensive support for running the WW operating system, referred to as Freya in this document, using cartridge images.

## Cartridge image operation

### Getting started (Simple)

1. Acquire a legal backup copy of a WW cartridge.
2. Launch it from swanshell as you would any other program.
3. Use an EXT port [RS-232 serial cable](https://consolemods.org/wiki/WonderSwan:RS-232_Serial_Cable) to communicate with FreyaOS.
4. To preserve any `rom0` file system changes (added/removed files), you *must* exit FreyaOS first. Press `START` to suspend the shell, then `Y3` to reboot.

### Getting started (AthenaBIOS)

[AthenaBIOS](https://github.com/OpenWitch/AthenaOS) is a clean-room, open source replacement for FreyaBIOS. It also allows using the nileswan's built-in USB port in place of an EXT port cable.

1. Connect an USB cable from the nileswan to the PC.
2. Acquire a copy of FreyaOS in one of the following ways:
    - Acquire a legal backup copy of a WW cartridge, then press `B` and select *Tools -> Witch -> Extract BIOS/OS*.
3. Create a new WW image by pressing `B` and selecting *Tools -> Witch -> Create image*. Select `AthenaBIOS (Native)`, the desired version of FreyaOS, then input the target image's desired filename.
4. Launch it from swanshell as you would any other program.
5. Use the USB serial port to communicate with FreyaOS.
6. To preserve any `rom0` file system changes (added/removed files), you *must* exit FreyaOS first. Press `START` to suspend the shell, then `Y3` to reboot.
7. To update/replace the BIOS or OS, press `B` and select *Tools -> Witch -> Replace BIOS* or *Replace OS*. New versions of swanshell contain updated AthenaBIOS versions, but they are not installed automatically.

Note that, being a reimplementation, AthenaBIOS may have compatibility issues compared to FreyaBIOS. If you run into any issues, please report them [here](https://github.com/OpenWitch/AthenaOS/issues). You can also swap between the two BIOSes at any time.

### Saving file system changes

Under FreyaOS, it is **required** to exit the OS in order to save WW file system contents to the removable storage card. This is because, while on-cartridge SRAM
(save data) is battery backed, on-cartridge PSRAM (file system contents) is not.

To exit FreyaOS while in the main menu, press `START` to suspend the shell, then `Y3` to reboot.
This operation will soft reset the cartridge, booting back into swanshell, which will proceed to save data to the card.

### Managing cartridge images

Managing cartridge images is done in the *Tools -> Witch* menu:

- *Replace BIOS*: upgrade/downgrade the BIOS on the selected cartridge image.
- *Replace OS*: upgrade/downgrade the OS on the selected cartridge image.
- *Create image*: creates a new WW cartridge image (requires a copy of the BIOS and OS).
- *Extract BIOS/OS*: extract BIOS/OS data from the selected cartridge image, which can be used when replacing them in other images or creating new ones.

## Direct .fx loading

Support for running `.fx` files directly is currently in development. Compatibility is very limited.

To test it, perform the following steps (no cartridge dumps are required):

1. Create a new folder, containing *only* the `.fx` program and any required `.il`/`.fr`/etc. files.
2. Open the folder in swanshell and launch the `.fx` program.

## Technical details

### Understanding Witch components

The WW operating system consists of two components:

- FreyaBIOS (last 64 KiB) - startup/recovery code, hardware abstraction layer, utility functions
- FreyaOS (second-to-last 64 KiB) - file system/process libraries, shell software

These components are copyrighted by Qute Corporation and cannot be distributed by the project. However, they can be extracted from
a legal backup of a WW cartridge image using the *Tools -> Witch -> Extract BIOS/OS* option. By doing so, one can use them
with other functions, such as creating new images or replacing the BIOS/OS version of existing images.

### AthenaBIOS details

AthenaBIOS is distributed
in two variants:

- Compatible: cartridge configured to closely mimic an official WW cartridge (NOR flash emulation enabled, serial communication via EXT port)
- Native: nileswan enhancements enabled (NOR flash emulation disabled for faster write performance, serial communication via on-cartridge USB port)

Here's a small comparison to help you decide which BIOS implementation to use:

|   | Freya | Athena Compatible | Athena Native |
| - | ----- | ----------------- | ------------- |
| License | Proprietary | [Open](https://github.com/OpenWitch/AthenaOS) | [Open](https://github.com/OpenWitch/AthenaOS) |
| Compatibility | 99.9% | 95% | 95% |
| Serial communication interface | EXT | EXT | USB |
| Flash write slowdowns | Yes | Yes | No |

## Limitations

There are a few caveats with regards to the limitations of WW hardware support on nileswan. These should not affect standard usage.

- The NOR flash emulation is only tested to the extent required by FreyaBIOS and AthenaBIOS's calls (INT 18h).
  - If an user-provided program tries to write to NOR flash directly, skipping the BIOS and going past these limitations, it may not behave correctly. For a variety of reasons, this is unlikely to occur in practice.
  - NOR flash erase operations do not actually set the data behind it to `0xFF`. This does not affect the FreyaOS file system in practice. This behaviour is also properly emulated by the Native variant of AthenaBIOS for its BIOS calls.
