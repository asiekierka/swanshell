---
title: 'Getting started'
weight: 5
---

## Lists and files

After successfully launching swanshell, the first screen displayed is the *file browser* view.

![The file browser view.](/swanshell/img/intro_file_browser.png)

This view is the primary method of interacting with data on the nileswan's storage card:

- The top bar displays the current directory's path.
- The bottom bar displays the currently displayed page, as well as the total number of pages. The contents of directories can span multiple pages. You can navigate between those pages by using the `X4` and `X2` buttons.
- The middle area displays the list of files on the current page of the current directory. You can use `X1` and `X3` to navigate between them.

To start using swanshell, use the `X` pad to navigate as described above, then press `A` to navigate into a directory or launch a file.

To exit a directory, press `B`. If you want to access additional features, you can press `START` to open a special submenu:

![The additional action menu.](/swanshell/img/intro_action_menu.png)

Many of these functions are explained on other pages of the swanshell documentation.

## Technical details

### File browser limitations

The maximum amount of supported files per directory is slightly over 1500.

Unicode file names are supported and will be displayed to the best of the font's
ability - the large font offering better support for non-English languages at this time.

### List navigations

The following buttons are used in all swanshell list views:

- The `X` pad (`X1`, `X2`, `X3`, `X4`) is used to navigate the contents of the list. Other lists (such as the settings menu) work the same way!
- `A` is used to select an option. For example, in the file browser, `A` may enter a subdirectory, or launch a `.wsc` file.
- `B` is used to navigate upwards. For example, in the file browser, `B` will exit a previously entered subdirectory.

Some views may have additional features mapped to the remaining buttons. For example, pressing `START` in the file browser brings up a menu
with additional actions. Alternatively, in the settings menu, `Y1` can be used to invoke further information about a specific option.

### PCv2 mapping

The PCv2 uses a different button layout than the WS/WSC. The mapping between the two is as follows:


| WS/WSC | PCv2 |
| ------ | ---- |
| `X1` - `X4` | `Up` / `Right` / `Down` / `Left` |
| `A` | `Circle` |
| `B` | `Clear` |
| `START` | `Esc` |
| `Y1` | `Pass` |
