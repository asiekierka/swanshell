---
title: 'Cartridge environment'
weight: 10
---

One way to run WW programs via swanshell is by using a **cartridge environment**. In this mode, a virtual cartridge is created containing the BIOS, OS and user-uploaded files on emulated NOR flash.

Pros:

* This mode provides the highest compatibility.
* The virtual cartridge data can be exchanged with PC emulators.

Cons:

* A legal backup copy of FreyaOS is currently required.
* Uploading programs requires using a PC with a file manager, like TransMagic, or terminal program.

## Which BIOS to use?

FreyaBIOS is the official WonderWitch BIOS. This option provides the highest compatibility, but also means no new features and bugfixes can be made available. [AthenaBIOS](https://github.com/OpenWitch/AthenaOS) is a clean-room, open source alternative for FreyaBIOS.

The most important difference for users is that FreyaBIOS requires an EXT port [RS-232 serial cable](https://consolemods.org/wiki/WonderSwan:RS-232_Serial_Cable) to be plugged into the console to upload and download files. Conversely, AthenaBIOS supports using the nileswan's on-cartridge USB port with a PC instead. **If you don't have an EXT port serial cable, you should use AthenaBIOS.**

## Getting started

### Using FreyaBIOS

1. Acquire a legal backup copy of a WW cartridge.
2. Launch it from swanshell as you would any other program.
  
### Using AthenaBIOS

1. Connect an USB cable from the nileswan to the PC.
2. Acquire a copy of FreyaOS in one of the following ways:
    - Acquire a legal backup copy of a WW cartridge, then press `B` and select *Tools -> Witch -> Extract BIOS/OS*.
3. Create a new WW image by pressing `B` and selecting *Tools -> Witch -> Create image*. Select `AthenaBIOS (Native)`, the desired version of FreyaOS, then input the target image's desired filename.
4. Launch it from swanshell as you would any other program.

## Saving file system

Under FreyaOS, it is **required** to exit the OS in order to save `/rom0` file system contents to the removable storage card - while on-cartridge SRAM (save data) is battery backed, on-cartridge PSRAM (file system contents) is not.

To exit FreyaOS while in the main menu, press `START` to suspend the shell, then `Y3` to reboot.
This operation will soft reset the cartridge, booting back into swanshell, which will proceed to save data to the card.

## Managing cartridge images

Managing cartridge images is done in the *Tools -> Witch* menu:

- *Replace BIOS*: upgrade/downgrade the BIOS on the selected cartridge image. You can use this to swap between FreyaBIOS and AthenaBIOS.
- *Replace OS*: upgrade/downgrade the OS on the selected cartridge image. Note that OS changes may cause a loss of file system data.
- *Create image*: creates a new WW cartridge image (requires a copy of the BIOS and OS).
- *Extract BIOS/OS*: extract BIOS/OS data from the selected cartridge image, which can be used when replacing them in other images or creating new images.

## AthenaBIOS variants

AthenaBIOS is distributed in two variants:

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
