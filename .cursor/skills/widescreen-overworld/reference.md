# Widescreen landmines and leftovers

## Transition tileattr

`overworld_tileattr` is a live 64x64 for the **current** 2x2. Mid-scroll (`submodule_index != 0`), that buffer is rebuilt for the destination while `BG2HOFS` is still on the old screen.

If fill reads live tileattr then:

- a vertical seam at the old/new boundary
- paths shift in Y
- one side uses the new area's lighting
- HUD can look like it only exists on one half

`OwWide_LiveTileattrOk` requires `submodule_index == 0` and `g_ow_wide_tileattr_area == (overworld_screen_index & 0x3f)`. Otherwise use the world-space cache.

`g_ow_wide_tileattr_area` is set in `OwWide_OnOverworldLoaded`, which must run from `Overworld_DrawQuadrantsAndOverlays` (screen actually loaded). `Overworld_LoadGFXAndScreenSize` runs earlier during `OverworldHandleTransitions` and would mark the new area as live while the camera is still on the old one. After LoadGFX, only `OwWide_PrefetchCamera`.

## Side-space clip

`ConfigurePpuSideSpace` used to do `uint16 HOFS - xstart`. When HOFS is left of `xstart` during a scroll, that underflows and the PPU treats a huge extra-left as "draw the loaded tilemap into the bar" (wrap / garbage). Cast to `int`, clamp `< 0` to 0. Fill then owns those pixels.

## Tile flip

SNES `0x4000` = hflip. In this PPU's bit layout, **set** means do not invert px:

```c
if (!(tile & 0x4000))
  px = 7 - px;
```

## Edges and special OW

- `overworld_screen_index >= 0x80`: special overworld, return -1 from `OwWide_ScreenAt`
- Small screens on the east (`area & 7 == 7`) or south (`area >> 3 == 7`) column/row: `mx`/`my` >= 32 is wrap, not a neighbor. Skip live tileattr and use cache (which will miss).

## Known leftovers

Leave these unless the user asks:

- No sprites / NPCs / items / rain-splash garnishes on the neighbor
- Neighbor tiles use the **current** VRAM + CGRAM (wrong tileset at Kakariko / woods / village borders)
- Dungeons still black-bar
- HUD is still 256px
- Projectiles can vanish (8-bit OAM)
- Mosaic, iris, mirror warp still use the original frame
