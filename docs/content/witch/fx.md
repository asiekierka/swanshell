---
title: 'Loading .fx files'
weight: 20
---

{{< hint type=important >}}
**This feature is experimental and in development.**

While many `.fx` games and programs already work correctly, many also exhibit visual bugs and even crashes.
{{< /hint >}}

The other way to run WW programs via swanshell is by **direct .fx loading** - simply extract a WW program onto the FAT file system and run it!

Pros:

* This mode is the most convenient to use.
* Neither FreyaBIOS nor FreyaOS are required - no cartridge backups necessary!

Cons:

* Many features are not yet implemented, such as:
  * Saving files from RAM to storage card
  * Multi-process support (ProcIL)
* Software compatibility is currently lower than the cartridge image method.

## Getting started

As an example, the open source game [PutiPati](https://www.asahi-net.or.jp/~cs8k-cyu/ww/putipati.html) by ABA Games will be used.

Create a **separate directory**[^sepdir] for your WW program. Extract all the files required by the program to this directory.

[^sepdir]: All files in the directory being launched are added to the emulated ROM, which can quickly go past the 640 KiB limit.

![The directory containing extracted PutiPati files on PC.](/swanshell/img/wwfxguide1.png)

Next, launch swanshell and navigate to the directory created in the previous step.

![The directory containing extracted PutiPati files in swanshell.](/swanshell/img/wwfxguide2.png)

Select the game's main `.fx` file, then press `A`. Enjoy!

## Common issues

### There's no sound / The game is complaining about missing "SoundIL"!

SoundIL is a library for playing music and sound bundled with the WonderWitch. While it is proprietary, games were allowed to redistribute it as part of their files - as such, you can find it pretty easily by searching for `sound102.il`. Place this file alongside the `.fx` of the game you wish to launch.