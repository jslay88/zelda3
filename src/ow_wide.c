#include "ow_wide.h"

#include <string.h>

#include "assets.h"
#include "config.h"
#include "overworld.h"
#include "snes/ppu.h"
#include "variables.h"
#include "zelda_rtl.h"

enum {
  kOwWideCacheSlots = 16,
  kOwWideMap16Count = 32 * 32,
};

typedef struct OwWideSlot {
  int16 screen;
  uint16 map16[kOwWideMap16Count];
} OwWideSlot;

static OwWideSlot g_ow_wide_cache[kOwWideCacheSlots];
static int g_ow_wide_lru;
static bool g_ow_wide_cache_inited;
static int g_ow_wide_tileattr_area = -1;

static void OwWide_InitCache(void) {
  if (g_ow_wide_cache_inited)
    return;
  for (int i = 0; i < kOwWideCacheSlots; i++)
    g_ow_wide_cache[i].screen = -1;
  g_ow_wide_cache_inited = true;
}

static int OwWide_ScreenAt(int world_x, int world_y) {
  if (BYTE(overworld_screen_index) >= 0x80)
    return -1;
  if (world_x < 0 || world_y < 0)
    return -1;
  int col = world_x >> 9;
  int row = world_y >> 9;
  if (col < 0 || col > 7 || row < 0 || row > 7)
    return -1;
  return (BYTE(overworld_screen_index) & ~0x3f) | (row << 3) | col;
}

static bool OwWide_DecompressScreen(int screen, uint16 *dst32) {
  MemBlk hi = kOverworld_Hibytes_Comp(screen);
  MemBlk lo = kOverworld_Lobytes_Comp(screen);
  if (!hi.ptr || !lo.ptr || hi.size == 0 || lo.size == 0)
    return false;

  uint8 ram_bak[0x800];
  memcpy(ram_bak, g_ram + 0x14000, sizeof(ram_bak));
  uint16 last = map16_decode_last;
  uint16 tmp = map16_decode_tmp;

  uint16 stride64[64 * 32];
  memset(stride64, 0, sizeof(stride64));
  Overworld_DecompressAndDrawOneQuadrant(stride64, screen);
  for (int y = 0; y < 32; y++)
    memcpy(dst32 + y * 32, stride64 + y * 64, 32 * sizeof(uint16));

  memcpy(g_ram + 0x14000, ram_bak, sizeof(ram_bak));
  map16_decode_last = last;
  map16_decode_tmp = tmp;
  return true;
}

static const uint16 *OwWide_CachedMap16(int screen) {
  OwWide_InitCache();
  if (screen < 0 || screen >= 160)
    return NULL;
  for (int i = 0; i < kOwWideCacheSlots; i++) {
    if (g_ow_wide_cache[i].screen == screen)
      return g_ow_wide_cache[i].map16;
  }
  OwWideSlot *slot = &g_ow_wide_cache[g_ow_wide_lru];
  g_ow_wide_lru = (g_ow_wide_lru + 1) % kOwWideCacheSlots;
  if (!OwWide_DecompressScreen(screen, slot->map16)) {
    slot->screen = -1;
    return NULL;
  }
  slot->screen = (int16)screen;
  return slot->map16;
}

static bool OwWide_LiveTileattrOk(void) {
  return submodule_index == 0 &&
         g_ow_wide_tileattr_area >= 0 &&
         BYTE(overworld_screen_index) < 0x80 &&
         (BYTE(overworld_screen_index) & 0x3f) == g_ow_wide_tileattr_area;
}

static bool OwWide_LookupMap16(int world_x, int world_y, uint16 *out) {
  // During a transition the live 2x2 is rebuilt for the destination
  // while the camera is still on the old screen. World-space cache
  // stays valid the whole way across.
  if (OwWide_LiveTileattrOk()) {
    int area = g_ow_wide_tileattr_area;
    int mx = (world_x - (int)kOverworld_OffsetBaseX[area]) >> 4;
    int my = (world_y - (int)kOverworld_OffsetBaseY[area]) >> 4;
    if (mx >= 0 && mx < 64 && my >= 0 && my < 64) {
      if (kOverworldMapIsSmall[area]) {
        if ((mx >= 32 && (area & 7) == 7) || (my >= 32 && (area >> 3) == 7))
          goto cached;
      }
      *out = overworld_tileattr[my * 64 + mx];
      return true;
    }
  }

cached:
  int screen = OwWide_ScreenAt(world_x, world_y);
  const uint16 *map = OwWide_CachedMap16(screen);
  if (!map)
    return false;
  *out = map[((world_y >> 4) & 31) * 32 + ((world_x >> 4) & 31)];
  return true;
}

static void OwWide_PrefetchAround(int world_x, int world_y) {
  for (int dy = -512; dy <= 512; dy += 512) {
    for (int dx = -512; dx <= 512; dx += 512)
      OwWide_CachedMap16(OwWide_ScreenAt(world_x + dx, world_y + dy));
  }
}

static uint8 OwWide_Decode4bpp(Ppu *ppu, int tileadr, uint16 tile, int x, int y) {
  int px = x & 7, py = y & 7;
  if (!(tile & 0x4000))
    px = 7 - px;
  if (tile & 0x8000)
    py = 7 - py;
  const uint16 *addr = &ppu->vram[(tileadr + (tile & 0x3ff) * 16 + py) & 0x7fff];
  uint32 bits = addr[0] | ((uint32)addr[8] << 16);
  int pixel = (bits >> px) & 1 | (bits >> (7 + px)) & 2 |
              (bits >> (14 + px)) & 4 | (bits >> (21 + px)) & 8;
  return pixel ? (uint8)(((tile & 0x1c00) >> 6) + pixel) : 0;
}

static uint8 OwWide_SampleNeighborBg2(Ppu *ppu, int world_x, int world_y) {
  uint16 map16;
  if (!OwWide_LookupMap16(world_x, world_y, &map16) || map16 >= 3752)
    return 0;
  const uint16 *map8 = kMap16ToMap8 + map16 * 4;
  uint16 tile = map8[((world_y & 8) >> 2) + ((world_x & 8) >> 3)];
  return OwWide_Decode4bpp(ppu, ppu->bgLayer[1].tileAdr, tile, world_x, world_y);
}

static uint8 OwWide_SampleBgLayer(Ppu *ppu, int layer, int sx, int sy) {
  BgLayer *bg = &ppu->bgLayer[layer];
  int x = sx + bg->hScroll;
  int y = sy + bg->vScroll;
  int sc_offs = bg->tilemapAdr + (((y >> 3) & 0x1f) << 5);
  if ((y & 0x100) && bg->tilemapHigher)
    sc_offs += bg->tilemapWider ? 0x800 : 0x400;
  if ((x & 0x100) && bg->tilemapWider)
    sc_offs += 0x400;
  uint16 tile = ppu->vram[(sc_offs + ((x >> 3) & 0x1f)) & 0x7fff];
  return OwWide_Decode4bpp(ppu, bg->tileAdr, tile, x, y);
}

static uint32 OwWide_Rgb(Ppu *ppu, uint32 color, const uint8 *map) {
  return (uint32)map[color & 0x1f] << 16 |
         (uint32)map[(color >> 5) & 0x1f] << 8 |
         (uint32)map[(color >> 10) & 0x1f];
}

// Matches PpuDrawWholeLine color math so rain/fog overlays reach the side bars.
static uint32 OwWide_Composite(Ppu *ppu, uint8 main_cgram, uint8 sub_cgram) {
  uint32 color = ppu->cgram[main_cgram];
  if (!(ppu->mathEnabled & (1 << 1)))
    return OwWide_Rgb(ppu, color, ppu->brightnessMult);

  uint32 r = color & 0x1f;
  uint32 g = (color >> 5) & 0x1f;
  uint32 b = (color >> 10) & 0x1f;
  const uint8 *color_map = ppu->brightnessMult;
  uint32 color2;
  if (ppu->addSubscreen) {
    if (sub_cgram != 0) {
      color2 = ppu->cgram[sub_cgram];
      if (ppu->halfColor)
        color_map = ppu->brightnessMultHalf;
    } else {
      color2 = (uint32)ppu->fixedColorR | (uint32)ppu->fixedColorG << 5 |
               (uint32)ppu->fixedColorB << 10;
    }
  } else {
    color2 = (uint32)ppu->fixedColorR | (uint32)ppu->fixedColorG << 5 |
             (uint32)ppu->fixedColorB << 10;
    if (ppu->halfColor)
      color_map = ppu->brightnessMultHalf;
  }
  uint32 r2 = color2 & 0x1f, g2 = (color2 >> 5) & 0x1f, b2 = (color2 >> 10) & 0x1f;
  if (ppu->subtractColor) {
    r = (r >= r2) ? r - r2 : 0;
    g = (g >= g2) ? g - g2 : 0;
    b = (b >= b2) ? b - b2 : 0;
  } else {
    r += r2;
    g += g2;
    b += b2;
  }
  return (uint32)color_map[b] | (uint32)color_map[g] << 8 | (uint32)color_map[r] << 16;
}

static void OwWide_FillSpan(uint8 *pixel_buffer, size_t pitch, int height,
                            int x0, int x1, int extra) {
  Ppu *ppu = g_zenv.ppu;
  if (x0 >= x1)
    return;
  int hofs = BG2HOFS_copy2;
  int vofs = BG2VOFS_copy2;
  bool overlay = (ppu->screenEnabled[1] & 1) != 0 && ppu->addSubscreen;
  for (int y = 0; y < height; y++) {
    uint32 *dst = (uint32 *)(pixel_buffer + (size_t)y * pitch);
    int world_y = vofs + y;
    for (int x = x0; x < x1; x++) {
      uint8 main_cgram = OwWide_SampleNeighborBg2(ppu, hofs - extra + x, world_y);
      uint8 sub_cgram = 0;
      if (overlay)
        sub_cgram = OwWide_SampleBgLayer(ppu, 0, x - extra, y);
      dst[x] = OwWide_Composite(ppu, main_cgram, sub_cgram);
    }
  }
}

void OwWide_PrefetchCamera(void) {
  int y = (int)BG2VOFS_copy2 + 112;
  OwWide_PrefetchAround((int)BG2HOFS_copy2 + 128, y);
  OwWide_PrefetchAround((int)BG2HOFS_copy2 - kPpuExtraLeftRight, y);
  OwWide_PrefetchAround((int)BG2HOFS_copy2 + 256 + kPpuExtraLeftRight, y);
}

void OwWide_OnOverworldLoaded(void) {
  if (BYTE(overworld_screen_index) < 0x80)
    g_ow_wide_tileattr_area = BYTE(overworld_screen_index) & 0x3f;
  else
    g_ow_wide_tileattr_area = -1;
  OwWide_PrefetchCamera();
}

void OwWide_FillFrame(uint8 *pixel_buffer, size_t pitch, int height) {
  if (!g_config.seamless_overworld || !pixel_buffer)
    return;
  Ppu *ppu = g_zenv.ppu;
  if (!ppu || !ppu->extraLeftRight || ppu->mode == 7 || ppu->forcedBlank)
    return;
  if (PpuGetCurrentRenderScale(ppu, ppu->renderFlags) != 1)
    return;

  int mod = main_module_index;
  if (mod == 14)
    mod = saved_module_for_menu;
  if (mod != 9)
    return;

  int extra = ppu->extraLeftRight;
  int left_cur = ppu->extraLeftCur;
  int right_cur = ppu->extraRightCur;
  int width = 256 + extra * 2;
  int hofs = BG2HOFS_copy2;
  int vofs = BG2VOFS_copy2;

  OwWide_PrefetchAround(hofs - extra, vofs + height / 2);
  OwWide_PrefetchAround(hofs + 256 + extra, vofs + height / 2);

  OwWide_FillSpan(pixel_buffer, pitch, height, 0, extra - left_cur, extra);
  OwWide_FillSpan(pixel_buffer, pitch, height, width - (extra - right_cur), width, extra);
}
