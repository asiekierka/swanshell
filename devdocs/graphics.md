# Graphics

## Tile allocation

- Tiles 0-503: 224x144 frame buffer
- Tiles 512-1015: wallpaper (WSC only)

## Palette allocation

- Palette 0: main drawing area palette
- Palette 1: main drawing area palette (selected)
- Palette 2: title bar palette
- Palette 3: status bar palette
- Palette 4: wallpaper (WSC only)
- Palette 8..15: general use

### Main drawing area palette

- Color 0: background (mono)
  - On WSC, screen 1 is used as background/wallpaper instead
- Color 1: foreground
- Colors 4-15: additional icon palette

### Bar palette

- Color 2, 3: background, foreground
  - If the bar is transparent (WSC), colors 0, 1 are used instead
- Color 4, 6, 8, 10, 12, 14: background stages (gradient mode)
- Color 5, 7, 9, 11, 13, 15: foreground stages (gradient mode)

### Additional views

#### Color picker (WS)

- Palette 8, color 3: preview shade
- Palette 9, 10, 11: R, G, B bar

#### Color picker (WSC)

- Palette 10, color 5: preview color
- Palette 10-11, color 6-7: R bar selector indicator border/fill
- Palette 10, color 8-15: R bar left half
- Palette 11, color 8-15: R bar right half
- Palette 12-13: G bar, as above
- Palette 14-15: B bar, as above
