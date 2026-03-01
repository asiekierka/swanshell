---
title: 'Remote shell'
weight: 50
---

swanshell provides a remote shell which is available via the USB serial port.

## Usage (Linux)

Install picocom, then run the following command, substituting `/dev/ttyACM0` with the serial port device used by nileswan:

    $ picocom --send-cmd "lrzsz-sx -vv -X" --receive-cmd "lrzsz-rx -vv -X" /dev/ttyACM0

Press ENTER once to activate the interactive shell.

    >

To list all available commands, type `help`. Some common operations are documented below:

### Loading programs from PC to console

Type `launch`, then press ENTER.

    > launch
    Awaiting XMODEM transfer

Press Ctrl-A, then Ctrl-S. Provide the path to your file:

    *** file: my-homebrew.ws

Press `ENTER`. The program should now be uploaded, then loaded.

This method has a few limitations:

- Only `.ws`, `.wsc`, `.pc2` and `.bfb` files are supported.
- Save data is not retained.

### Loading programs from PC to storage card

To upload a program, type `upload [path]`, then press ENTER. You can use `cd` to enter a directory beforehand.

    > cd Homebrew
    > upload /my-homebrew.ws
    Awaiting XMODEM transfer

Press Ctrl-A, then Ctrl-S. Provide the path to your file:

    *** file: my-homebrew.ws

Press `ENTER`. The program should now be uploaded. To start a program, type `launch [path]`, then press ENTER:

    Saving file........
    
    > launch /my-homebrew.ws

This method uses the exact same codepath as swanshell's user interface; as such, additional file types like `.fx` are supported, and save data is retained.

## Loading programs without remote shell

In the user interface, selecting `Tools` -> `Launch via XMODEM` allows loading programs via the USB serial port without using the shell.

The transfer protocol used is XMODEM. On Windows, you can use [Tera Term](https://teratermproject.github.io/index-en.html) to send files via XMODEM; on Linux, `minicom` and `lrzsz` are viable solutions.
