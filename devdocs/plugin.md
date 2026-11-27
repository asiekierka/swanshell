# Plugin format

## Memory map

Plugins are loaded into the topmost (final) banks of PSRAM. This allows using PSRAM bank 0 onwards by the program, just like swanshell itself does.

The IPC area is populated with regards to [the memory map](https://www.49bitcat.com/docs/nileswan/developer/memory_map/#ipc-buffer). The user data area (0x0E0..0x1FF) stores the path of the file requested by sa.
