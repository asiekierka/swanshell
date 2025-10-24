# swanshell

The primary and official file selector/launcher program for [nileswan](https://49bitcat.com/docs/nileswan/).

Documentation is available [here](https://docs.asie.pl/swanshell/).

## Building

### Requirements

- [Wonderful Toolchain](https://wonderful.asie.pl/wiki/doku.php?id=getting_started)
  - `wf-pacman -S target-wswan wf-superfamiconv wf-zx0-salvador`
- Python >= 3.11 + uv

### Instructions

    make

To build auxilliary files as well:

    make dist

To force a font rebuild:

    make -B fonts

## Contributing

### Translations

Translations are maintained using [Weblate](https://weblate.asie.pl/projects/49bitcat/swanshell/).

The process has not been well-tested yet. Feel free to open an issue for any further inquiries.

## License

- `docs/`: CC BY-SA 4.0
- `src/`: GPLv3
