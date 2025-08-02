---
title: 'Extras'
weight: 60
---

## Loading programs via USB

The `Tools` -> `Launch via XMODEM` option allows loading programs via nileswan's USB port. `.ws`/`.wsc` and `.bfb` programs can be loaded this way.

The protocol used is XMODEM. On Windows, you can use [Tera Term](https://teratermproject.github.io/index-en.html) to send files via XMODEM; on Linux, `minicom` and `lrzsz` are viable solutions.

## Controller mode

The *controller mode* allows using your console as an USB HID controller.

Press `START`, then select `Tools` -> `Controller mode` to activate.

Note that, due to the console's non-standard button layout, only games and programs which support custom controller button bindings should be expected to work correctly.