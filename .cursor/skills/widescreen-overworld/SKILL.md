---
name: widescreen-overworld
description: Seamless 16:9 overworld neighbor-screen fill. Use when working on black bars, ExtendedAspectRatio, SeamlessOverworld, ow_wide, overworld transitions, or PPU extra side space.
---

# Widescreen overworld

Original 16:9 only reveals extra pixels **inside** the loaded screen. At the edge, `ConfigurePpuSideSpace` clips and the PPU blacks the bars. `OwWide_FillFrame` draws neighbor map16 into those pixels after the scanline pass.

## Files

- `src/ow_wide.c` / `src/ow_wide.h` (cache + fill)
- `src/overworld.c` (`OwWide_OnOverworldLoaded`, `OwWide_PrefetchCamera`)
- `src/zelda_rtl.c` (`OwWide_FillFrame`, signed `ConfigurePpuSideSpace`)
- `src/config.c` / `zelda3.ini` (`SeamlessOverworld`)

## How it works

1. Decompress neighbor map16 via `Overworld_DecompressAndDrawOneQuadrant` into a 16-slot cache (32x32 per OW cell). Save/restore `g_ram + 0x14000` and `map16_decode_*`.
2. World cell: `x >> 9`, `y >> 9` on the 8x8 grid. Light/dark from `overworld_screen_index & ~0x3f`.
3. After `ZeldaDrawPpuFrame` scanlines, fill `extra - extraLeftCur` on the left and the matching right span.
4. Rain/fog: sample BG1 + the same color math as `PpuDrawWholeLine` (`OwWide_Composite`).

## Do not regress these

Full list and known gaps: [reference.md](reference.md).

Short version: cache-only during transitions; never stamp tileattr from `LoadGFX`; signed HOFS clamp; tile `0x4000` is "don't invert px".

## Verify

`ExtendedAspectRatio = 16:9`, walk edges, cut grass, cross a transition. You want neighbor tiles the whole way, no center seam / lighting split / path Y-shift.
