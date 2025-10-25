---
title: 'Installation'
weight: 0
---

Before installing swanshell, make sure to format the removable storage card, sometimes referred to as the TF card, which you will put in the nileswan cartridge. FAT16 and FAT32 are both supported.

{{< hint type=warning >}}
By default, Windows and macOS operating systems will format cards larger than 32 GB as exFAT, which is *not* supported.

To solve this, follow the [storage card formatting](https://49bitcat.com/docs/nileswan/user/troubleshooting/storage-card-formatting/) guide.
{{< /hint >}}

After formatting the removable storage card, download the latest swanshell ZIP from [GitHub releases](github.com/asiekierka/swanshell/releases) and extract its contents to the root directory of the card. After doing so, you should have a `NILESWAN` directory on the drive.

Finally, eject the card from your computer and insert it into the nileswan cartridge. From here, you may follow to the [Getting started](../introduction) guide.

## Upgrading

To upgrade swanshell, simply unpack the newest version
of the menu software to the root directory of the cartridge's storage card.

As newer features in swanshell may rely on newly introduced cartridge functionality, it is recommended to periodically install the latest
nileswan firmware update. You can find the `.ws` file used to do so [here](https://github.com/49bitcat/nileswan/releases).
