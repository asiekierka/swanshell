---
title: 'Witch support'
weight: 45
---

{{< hint type=important >}}
WonderWitch is a product of Qute Corporation. swanshell is not endorsed or supported by Qute Corporation.

**Please refrain** from contacting Qute Corporation with any inquiries regarding running WonderWitch software on nileswan.
{{< /hint >}}

swanshell has extensive support for running the WW's operating system, referred to as Freya in this document, using cartridge images.
Support for running `.fx` files directly is also in development.

## Cartridge image support

### Exiting / Saving data

When using FreyaOS, exiting the OS is required to properly save data to the removable storage card. This is required as while SRAM (used for save data) is
battery backed, the PSRAM (used for cartridge data) is not. 

To do this, in the main shell menu, press `START` to suspend the shell, then `Y3` to reboot the cartridge. This operation will load swanshell, which will proceed to save data to the card.

### Understanding Witch components

The WW operating system consists of two components:

- FreyaBIOS (last 64 KiB) - startup/recovery code, hardware abstraction layer, utility functions
- FreyaOS (second-last 64 KiB) - file system/process libraries, shell software

These components are copyrighted by Qute Corporation and cannot be distributed by the project. However, they can be extracted from
a legal backup of a WW cartridge image using the *Tools -> Witch -> Extract BIOS/OS* option. By doing so, one can use them
with other functions, such as creating new images or replacing the BIOS/OS version of existing images.

### AthenaBIOS

[AthenaBIOS](https://github.com/OpenWitch/AthenaOS) is a clean-room, open source replacement for FreyaBIOS. It is distributed
in two variants:

- Compatible: cartridge configured to closely mimic an official WW cartridge (NOR flash emulation enabled, serial communication via EXT port)
- Native: nileswan enhancements enabled (NOR flash emulation disabled for faster write performance, serial communication via on-cartridge USB port)

Here's a small comparison to help you decide which BIOS implementation to use:

|   | Freya | Athena Compatible | Athena Native |
| - | ----- | ----------------- | ------------- |
| License | Proprietary | Open | Open |
| Compatibility | 99.9% | 95% | 94% |
| Serial communication interface | EXT | EXT | USB |
| Flash write slowdowns | Yes | Yes | No |

### Managing cartridge images

Managing cartridge images is done in the *Tools -> Witch* menu:

- *Replace BIOS*: upgrade/downgrade the BIOS on the selected cartridge image.
- *Replace OS*: upgrade/downgrade the OS on the selected cartridge image.
- *Create image*: creates a new WW cartridge image (requires a copy of the BIOS and OS).
- *Extract BIOS/OS*: extract BIOS/OS data from the selected cartridge image, which can be used when replacing them in other images or creating new ones.

## Limitations

There are a few caveats with regards to WW hardware support. You should not run into them as a standard user, however:

- The NOR flash emulation is only tested with regards to FreyaBIOS and AthenaBIOS's requirements.
  - If an user-provided program tries to write to NOR flash directly (unlikely), the specific commands it uses may not be supported.
  - Flash erases do not actually set the data behind them to `0xFF`. This does not affect the FreyaOS file system driver; it is also re-implemented using RAM writes by the Native variant of AthenaBIOS.
