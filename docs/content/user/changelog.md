---
title: 'Changelog'
weight: 100
---

## swanshell 1.1.2 (19th March 2026)

- Added: New USB shell commands: `cat`, `echo`, `pwd`.
- Added: Option to scroll long file names in the file selector. (WIP)
- Fixed: Regression in swanshell 1.1.0 which led to slower XMODEM transfer start times.
- Fixed: RTC date/time settings menu occasionally reading corrupt information.

## swanshell 1.1.1 (4th March 2026)

- Changed: Ctrl+C in the USB shell now activates interactive mode.
- Changed: Updated Chinese (Simplified) translation. (PECO)
- Fixed: Regression in EEPROM mode initialization.

## swanshell 1.1.0 (4th March 2026)

- Added: Enhanced system information menu.
- Added: Enhanced USB shell:
  - New commands: `cd`, `download`, `help`, `ls`, `mkdir`, `reboot`, `rm`, `upload`.
  - The Ctrl+C keybind is now supported.
  - The `launch` command now accepts an optional path.
- Added: On firmware 1.1.0, missing/low battery warnings.
- Added: Saving `/ram0` contents when loading `.fx` programs directly.
- Added: Status icons - the cartridge's status is now communicated visually in the bottom right corner.
- Added: The option to hide save files.
- Changed: On firmware 1.1.0+, inserting a removed storage card will trigger a cartridge reboot.
- Changed: Slightly improved battery draw in the menu when no USB cable is connected.
- Changed: Text viewer improvements:
  - Minor performance improvements.
  - The text viewer now shows an approximate percentage position in the file being read.
  - The text viewer now supports going back in a file.
  - The text viewer now supports both Shift-JIS and UTF-8 encoded files, with automatic detection.
- Changed: Tweaked visual padding for pop-up dialogs.
- Fixed: Certain memory initialization issues when launching a program.
- Fixed: Reliability problems when using XMODEM file transfers within the USB shell.
- Other minor improvements.

## swanshell 1.0.4 (23rd February 2026)

- Changed: A warning is now displayed for overdumped save data.
- Fixed: A soft reset patch was incorrectly applied to certain cartridge images.

## swanshell 1.0.3 (23rd February 2026)

- Changed: Minor translation updates.
- Fixed: Some cartridge images were incorrectly detected as containing on-board NOR flash.
- Fixed: The text viewer should now correctly read files over 64 KiB.

## swanshell 1.0.2 (14th February 2026)

- Added: Input repeat delay and rate configuration.
- Added: New translations:
  - Chinese (Simplified) (PECO)
- Added: Simple text viewer.
- Added: Support for Chinese and partial support for Korean characters.
- Changed: On a new installation, the user is automatically prompted for their preferred language.
- Changed: Question marks are now used for characters without a matching glyph in the font.
- Changed: The display fonts are now stored in separate files.
- Fixed: The "Hide icons" option not saving or loading correctly.
- Minor performance and reliability improvements.

## swanshell 1.0.1 (7th February 2026)

- Added: New translations:
  - German (Generic)
  - French (zeroballoons)
  - Spanish (Fossil)
- Changed: The file selector now remembers its position in the list after deleting a file.
- Minor performance and reliability improvements.

## swanshell 1.0.0 (3rd January 2026)

- Initial release.
