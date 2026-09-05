---
title: 'Frequently asked questions'
weight: 95
---

## Usage

### How do I launch a WW (.fx) program?

There are two methods:

- You can simply launch the extracted .fx program from the file browser. See [Loading .fx files](../../witch/fx/) for more information.
- You can use a WW cartridge environment. See [Cartridge environment](../../witch/image/) for more information.

The direct .fx method is easier to use, but it is experimental; most, but not all programs, should operate correctly with this method.

### How do I send/receive files from a WW cartridge image?

There are two methods:

- If your cartridge BIOS is AthenaBIOS (Native), use the USB port on the nileswan cartridge.
- If your cartridge BIOS is FreyaBIOS (official/unmodified BIOS) or AthenaBIOS (Compatible), use the EXT port on the console with a serial port adapter.

See [Cartridge environment](../../witch/image/) for more information.

## Troubleshooting

### When powering off a WW cartridge, save data or PSRAM state is corrupted.

The nileswan cartridge's battery can retain SRAM/EEPROM contents, but not PSRAM/emulated NOR flash contents. To correctly save NOR flash contents
when exiting a WW cartridge, you must reboot the OS by entering the main menu, pressing `START` to suspend the shell, then `Y3` to reboot.

See [Cartridge environment](../../witch/image/) for more information.
