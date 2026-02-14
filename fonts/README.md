# swanshell font format

## Header

| Offset | Size | Description |
| ------ | ---- | ----------- |
| 0 | 2 | Magic: `Sf` |
| 2 | 2 | Version: `0x0100` |
| 4 | 2 | Offset from beginning of file to font lookup table |
| 6 | 2 | Maximum codepoint, shifted right by 8 |
| 8 | 3 | Font lookup table-style pointer to unknown glyph value |
| 11 | 1 | Font height, in pixels (8 or 16) |

## Font lookup table

3-byte entries featuring the offset from beginning of file to font entry table for each
consecutive 256 codepoints from 0 to the maximum codepoint.

## Font entry table

Each font entry table must be placed within the bounds of consecutive 64 KiB blocks
from the start of the file. Crossing the 64 KiB boundary is not allowed.

### Dense font entry table

N = 0 .. 255

| Offset | Size | Description |
| ------ | ---- | ----------- |
| 0 | 2 | `0xFFFF` |
| 2 + 4N | 2 | Offset from beginning of file to given font entry. `0` if not present. |
| 2 + 4N + 2 | 1 | Bits 7-4: Y offset, bits 3-0: X offset |
| 2 + 4N + 3 | 1 | Bits 7-4: height, bits 3-0: width |

### Sparse font entry table

N = 0 .. number of glyphs minus one

| Offset | Size | Description |
| ------ | ---- | ----------- |
| 0 | 2 | Number of glyphs |
| 2 + 5N | 1 | Low 8 bits of font entry |
| 2 + 5N + 1 | 2 | Offset from beginning of file to given font entry. `0` if not present. |
| 2 + 5N + 3 | 1 | Bits 7-4: Y offset, bits 3-0: X offset |
| 2 + 5N + 4 | 1 | Bits 7-4: height, bits 3-0: width |

## Font entry

(Width x Height) consecutive bits of pixel data.
