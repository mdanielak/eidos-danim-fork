# Pixel art filter

Settings > Display > Pixel art filter changes magnification immediately:

- **None** (default): original pixels.
- **Light**: bilinear sampling, with alpha-weighted colours at transparent
  edges to avoid dark halos.
- **Strong**: Scale3x corner reconstruction. It uses neighbouring colours
  to round stair steps while retaining the original palette.

The GPU applies this to recognized Pokemon and effect sprites in one-screen battles.
The one-screen interface, item icons and other game
textures retain their existing sampling. It does not change the ROM or save data. The software
renderer retains its original pixels. At native size or when reducing the
image, the filter leaves the texels unchanged.

The setting is `pixel_filter = none|light|strong` in `[display]` of
`settings.ini`. Existing files without this key select None. Changing it
does not require restarting the game.

The Scale3x rules are documented by the algorithm's authors at
https://www.scale2x.it/algorithm. The shader is in
`engine/platform/pixel_filter.h`.
