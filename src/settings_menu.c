#include "settings_menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assets.h"
#include "audio.h"
#include "config.h"
#include "features.h"
#include "nmi.h"
#include "overworld.h"
#include "snes/ppu.h"
#include "util.h"
#include "variables.h"
#include "zelda_rtl.h"

enum {
  kSetTab_Gameplay = 0,
  kSetTab_Graphics,
  kSetTab_Sound,
  kSetTab_Dev,
  kSetTab_Count,
};

enum {
  kSetUi_Closed = 0,
  kSetUi_Menu,
  kSetUi_PauseChooser,
};

enum {
  kSid_FeatureBit = 0,
  kSid_Bool,
  kSid_Aspect,
  kSid_Fullscreen,
  kSid_WindowScale,
  kSid_OutputMethod,
  kSid_AudioFreq,
  kSid_AudioSamples,
  kSid_AudioCh,
  kSid_MsuMode,
  kSid_MsuVol,
  kSid_PathShader,
  kSid_PathLink,
  kSid_Language,
  kSid_DevOverlay,
  kSid_DevInfo,
  kSid_CheatLife,
  kSid_CheatKeys,
  kSid_CheatGear,
};

enum {
  kRowF_Action = 2,
  kRowF_Info = 4,
};

typedef struct SetRow {
  const char *label;
  const char *help;
  uint8 sid;
  uint8 flags;
  uint32 extra;
} SetRow;

int g_dev_force_overlay = -1;
bool g_dev_skip_live_tileattr;
bool g_dev_skip_fill;

static uint8 g_ui;
static int g_from;
static int g_tab;
static int g_row;
static int g_scroll;
static int g_chooser_row;
static bool g_dirty;
static int g_help_tab = -1;
static int g_help_row = -1;
static int g_help_tick;

static const char *kTabNames[kSetTab_Count] = {
  "GAMEPLAY", "GRAPHICS", "SOUND", "DEV"
};

static const int kForceOverlayIds[] = { -1, 0x9f, 0x96, 0x9d, 0x9e, 0x97 };
static const char *kForceOverlayNames[] = {
  "OFF", "RAIN", "SKY", "FOREST", "WOODS", "GROVE"
};

static const SetRow kRowsGameplay[] = {
  { "ITEM SWITCH L/R", "Hold X/L/R in inventory to assign items", kSid_FeatureBit, 0, kFeatures0_SwitchLR },
  { "LIMIT L/R TO 4", "Cycle only the first four items", kSid_FeatureBit, 0, kFeatures0_SwitchLRLimit },
  { "TURN WHILE DASHING", "Steer while running with the pegasus boots", kSid_FeatureBit, 0, kFeatures0_TurnWhileDashing },
  { "MIRROR TO DARK WORLD", "Mirror works from light to dark", kSid_FeatureBit, 0, kFeatures0_MirrorToDarkworld },
  { "SWORD COLLECTS ITEMS", "Slash hearts and pickups", kSid_FeatureBit, 0, kFeatures0_CollectItemsWithSword },
  { "SWORD BREAKS POTS", "Sword smashes indoor pots", kSid_FeatureBit, 0, kFeatures0_BreakPotsWithSword },
  { "DISABLE HEART BEEP", "Mute the low-health beep", kSid_FeatureBit, 0, kFeatures0_DisableLowHealthBeep },
  { "SKIP INTRO ON KEY", "Skip the opening on button press", kSid_FeatureBit, 0, kFeatures0_SkipIntroOnKeypress },
  { "MAX ITEMS YELLOW", "Orange/yellow when rupees bombs arrows are max", kSid_FeatureBit, 0, kFeatures0_ShowMaxItemsInYellow },
  { "MORE ACTIVE BOMBS", "Four bombs out at once", kSid_FeatureBit, 0, kFeatures0_MoreActiveBombs },
  { "LARGER WALLET", "Carry 9999 rupees", kSid_FeatureBit, 0, kFeatures0_CarryMoreRupees },
  { "MISC BUG FIXES", "Small engine fixes", kSid_FeatureBit, 0, kFeatures0_MiscBugFixes },
  { "GAME CHANGING FIXES", "Fixes that change behavior", kSid_FeatureBit, 0, kFeatures0_GameChangingBugFixes },
  { "CANCEL BIRD", "X cancels bird travel", kSid_FeatureBit, 0, kFeatures0_CancelBirdTravel },
  { "LANGUAGE", "Dialogue language if assets exist", kSid_Language, 0, 0 },
};

static const SetRow kRowsGraphics[] = {
  { "ASPECT RATIO", "4:3 / 16:9 / 16:10 side bars", kSid_Aspect, 0, 0 },
  { "SEAMLESS OVERWORLD", "Draw neighbor screens in the side bars", kSid_Bool, 0, 1 },
  { "EXTEND Y", "240 lines instead of 224", kSid_Bool, 0, 2 },
  { "NEW RENDERER", "Optimized SNES PPU", kSid_Bool, 0, 3 },
  { "ENHANCED MODE7", "Higher-res world map", kSid_Bool, 0, 4 },
  { "NO SPRITE LIMIT", "No per-scanline sprite drop", kSid_Bool, 0, 5 },
  { "STRETCH", "Ignore aspect ratio", kSid_Bool, 0, 6 },
  { "LINEAR FILTER", "Softer pixels", kSid_Bool, 0, 7 },
  { "DIM FLASHES", "Virtual Console style flashes", kSid_FeatureBit, 0, kFeatures0_DimFlashes },
  { "WINDOW SCALE", "1x to 10x", kSid_WindowScale, 0, 0 },
  { "FULLSCREEN", "Windowed / borderless / exclusive", kSid_Fullscreen, 0, 0 },
  { "AUTOSAVE", "Save state on quit, reload on start", kSid_Bool, 0, 8 },
  { "PERF IN TITLE", "Show FPS in the window title", kSid_Bool, 0, 9 },
  { "DISABLE FRAME DELAY", "Skip SDL_Delay each frame", kSid_Bool, 0, 10 },
  { "OUTPUT METHOD", "SDL / software / GL", kSid_OutputMethod, 0, 0 },
  { "SHADER", "Path in zelda3.user.ini", kSid_PathShader, kRowF_Info, 0 },
  { "LINK GRAPHICS", "ZSPR path in zelda3.user.ini", kSid_PathLink, kRowF_Info, 0 },
};

static const SetRow kRowsSound[] = {
  { "ENABLE AUDIO", "Open or close the audio device", kSid_Bool, 0, 11 },
  { "CHANNELS", "Mono or stereo", kSid_AudioCh, 0, 0 },
  { "FREQUENCY", "Output sample rate", kSid_AudioFreq, 0, 0 },
  { "SAMPLES", "Buffer size", kSid_AudioSamples, 0, 0 },
  { "MSU", "Off / MSU / deluxe / opuz", kSid_MsuMode, 0, 0 },
  { "RESUME MSU", "Remember MSU position per area", kSid_Bool, 0, 12 },
  { "MSU VOLUME", "0 to 100", kSid_MsuVol, 0, 0 },
};

static const SetRow kRowsDev[] = {
  { "OVERLAY ID", "Current overlay_index", kSid_DevInfo, kRowF_Info, 0 },
  { "OW SCREEN", "overworld_screen_index", kSid_DevInfo, kRowF_Info, 1 },
  { "MODULE", "main / sub", kSid_DevInfo, kRowF_Info, 2 },
  { "SIDE SPACE", "PPU extra left/right", kSid_DevInfo, kRowF_Info, 3 },
  { "FORCE OVERLAY", "Applies as soon as you change it", kSid_DevOverlay, 0, 0 },
  { "SEAMLESS OW", "Neighbor fill on/off", kSid_Bool, 0, 1 },
  { "CACHE TILEATTR ONLY", "Skip live 2x2 during fill", kSid_Bool, 0, 13 },
  { "FILL SIDES", "Software neighbor fill on/off", kSid_Bool, 0, 14 },
  { "FILL HEARTS", "Same as the W cheat", kSid_CheatLife, kRowF_Action, 0 },
  { "FILL KEYS", "Same as the O cheat", kSid_CheatKeys, kRowF_Action, 0 },
  { "FILL GEAR", "Same as Shift+W", kSid_CheatGear, kRowF_Action, 0 },
};

static bool Set_DevMenuEnabled(void) {
  static int cached = -1;
  if (cached < 0) {
    const char *e = getenv("ENABLE_DEV_MENU");
    cached = (e && (StringEqualsNoCase(e, "1") || StringEqualsNoCase(e, "true"))) ? 1 : 0;
  }
  return cached != 0;
}

static int Set_VisibleTabCount(void) {
  return Set_DevMenuEnabled() ? kSetTab_Count : kSetTab_Dev;
}

static const SetRow *Set_TabRows(int tab, int *count) {
  switch (tab) {
  case kSetTab_Gameplay: *count = (int)(sizeof(kRowsGameplay) / sizeof(kRowsGameplay[0])); return kRowsGameplay;
  case kSetTab_Graphics: *count = (int)(sizeof(kRowsGraphics) / sizeof(kRowsGraphics[0])); return kRowsGraphics;
  case kSetTab_Sound: *count = (int)(sizeof(kRowsSound) / sizeof(kRowsSound[0])); return kRowsSound;
  default: *count = (int)(sizeof(kRowsDev) / sizeof(kRowsDev[0])); return kRowsDev;
  }
}

static bool *Set_BoolPtr(uint32 extra) {
  switch (extra) {
  case 1: return &g_config.seamless_overworld;
  case 2: return &g_config.extend_y;
  case 3: return &g_config.new_renderer;
  case 4: return &g_config.enhanced_mode7;
  case 5: return &g_config.no_sprite_limits;
  case 6: return &g_config.ignore_aspect_ratio;
  case 7: return &g_config.linear_filtering;
  case 8: return &g_config.autosave;
  case 9: return &g_config.display_perf_title;
  case 10: return &g_config.disable_frame_delay;
  case 11: return &g_config.enable_audio;
  case 12: return &g_config.resume_msu;
  case 13: return &g_dev_skip_live_tileattr;
  case 14: return &g_dev_skip_fill;
  default: return NULL;
  }
}

static int Set_AspectIndex(void) {
  int e = g_config.extended_aspect_ratio;
  int h = g_config.extend_y ? 240 : 224;
  int e169 = (h * 16 / 9 - 256) / 2;
  int e1610 = (h * 16 / 10 - 256) / 2;
  if (e == 0) return 0;
  if (e == e1610) return 2;
  (void)e169;
  return 1;
}

static void Set_SetAspect(int idx) {
  int h = g_config.extend_y ? 240 : 224;
  if (idx <= 0) {
    g_config.extended_aspect_ratio = 0;
    g_config.features0 &= ~(kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes);
  } else if (idx == 2) {
    g_config.extended_aspect_ratio = (uint8)((h * 16 / 10 - 256) / 2);
    g_config.features0 |= kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes;
  } else {
    g_config.extended_aspect_ratio = (uint8)((h * 16 / 9 - 256) / 2);
    g_config.features0 |= kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes;
  }
}

static char g_lang_storage[16];

static void Set_CycleLanguage(int dir) {
  char names[16][16];
  int n = 0;
  for (int i = 0; i < 16; i++) {
    MemBlk mb = kDialogueMap(i);
    if (!mb.ptr)
      break;
    MemBlk name = FindIndexInMemblk(mb, 0);
    if (!name.size)
      continue;
    int len = name.size < 15 ? (int)name.size : 15;
    memcpy(names[n], name.ptr, len);
    names[n][len] = 0;
    n++;
  }
  if (!n)
    return;
  int cur = 0;
  const char *lang = g_config.language ? g_config.language : "US";
  for (int i = 0; i < n; i++) {
    if (StringEqualsNoCase(names[i], lang))
      cur = i;
  }
  cur += (dir < 0) ? -1 : 1;
  if (cur < 0)
    cur = n - 1;
  if (cur >= n)
    cur = 0;
  memcpy(g_lang_storage, names[cur], 16);
  g_config.language = g_lang_storage;
}

static bool Set_OverlayCanApply(void) {
  uint8 mod = main_module_index;
  if (mod == 14)
    mod = saved_module_for_menu;
  if (mod != 9)
    return false;
  return BYTE(overworld_screen_index) < 0x80;
}

static void Set_ApplyOverlay(void) {
  if (!Set_OverlayCanApply())
    return;
  uint8 bak_sub = submodule_index;
  Overworld_LoadOverlays2();
  submodule_index = bak_sub;
  NMI_UpdateSubscreenOverlay();
  nmi_subroutine_index = 0;
  nmi_disable_core_updates = 0;
}

static void Set_ReturnToGameplay(void) {
  g_ui = kSetUi_Closed;
  main_module_index = saved_module_for_menu;
  submodule_index = 0;
  choice_in_multiselect_box = choice_in_multiselect_box_bak;
}

void Settings_ApplyLive(void) {
  g_wanted_zelda_features = g_config.features0;
  enhanced_features0 = g_config.features0;
  msu_volume = (uint8)((g_config.msuvolume * 255) / 100);
  ZeldaSetLanguage(g_config.language);
  ZeldaEnableMsu(g_config.enable_msu);
  Settings_ApplyVideo();
}

static void Set_MarkDirty(void) {
  g_dirty = true;
  Settings_ApplyLive();
  WriteUserConfigFile();
}

static void Set_Format(const SetRow *r, char *buf, size_t n) {
  bool *bp;
  switch (r->sid) {
  case kSid_FeatureBit:
    snprintf(buf, n, "%s", (g_config.features0 & r->extra) ? "ON" : "OFF");
    break;
  case kSid_Bool:
    bp = Set_BoolPtr(r->extra);
    snprintf(buf, n, "%s", (bp && *bp) ? "ON" : "OFF");
    break;
  case kSid_Aspect: {
    static const char *names[] = { "4:3", "16:9", "16:10" };
    snprintf(buf, n, "%s", names[Set_AspectIndex()]);
    break;
  }
  case kSid_Fullscreen: {
    static const char *names[] = { "WINDOW", "BORDERLESS", "FULL" };
    snprintf(buf, n, "%s", names[g_config.fullscreen > 2 ? 0 : g_config.fullscreen]);
    break;
  }
  case kSid_WindowScale:
    snprintf(buf, n, "%dx", g_config.window_scale ? g_config.window_scale : 2);
    break;
  case kSid_OutputMethod: {
    static const char *names[] = { "SDL", "SDL-SW", "OPENGL", "GLES" };
    snprintf(buf, n, "%s", names[g_config.output_method > 3 ? 0 : g_config.output_method]);
    break;
  }
  case kSid_AudioFreq:
    snprintf(buf, n, "%d", g_config.audio_freq);
    break;
  case kSid_AudioSamples:
    snprintf(buf, n, "%d", g_config.audio_samples);
    break;
  case kSid_AudioCh:
    snprintf(buf, n, "%s", g_config.audio_channels == 1 ? "MONO" : "STEREO");
    break;
  case kSid_MsuMode: {
    uint8 m = g_config.enable_msu;
    const char *s = "OFF";
    if (m == (kMsuEnabled_MsuDeluxe | kMsuEnabled_Opuz)) s = "DX-OPUZ";
    else if (m & kMsuEnabled_Opuz) s = "OPUZ";
    else if (m & kMsuEnabled_MsuDeluxe) s = "DELUXE";
    else if (m) s = "MSU";
    snprintf(buf, n, "%s", s);
    break;
  }
  case kSid_MsuVol:
    snprintf(buf, n, "%d", g_config.msuvolume);
    break;
  case kSid_PathShader:
    snprintf(buf, n, "%s", (g_config.shader && *g_config.shader) ? g_config.shader : "(none)");
    break;
  case kSid_PathLink:
    snprintf(buf, n, "%s", g_config.link_graphics ? g_config.link_graphics : "(none)");
    break;
  case kSid_Language:
    snprintf(buf, n, "%s", g_config.language ? g_config.language : "US");
    break;
  case kSid_DevOverlay: {
    const char *s = "OFF";
    for (int i = 0; i < 6; i++) {
      if (kForceOverlayIds[i] == g_dev_force_overlay)
        s = kForceOverlayNames[i];
    }
    snprintf(buf, n, "%s", s);
    break;
  }
  case kSid_DevInfo:
    if (r->extra == 0) snprintf(buf, n, "%02X", BYTE(overlay_index));
    else if (r->extra == 1) snprintf(buf, n, "%02X", BYTE(overworld_screen_index));
    else if (r->extra == 2) snprintf(buf, n, "%d/%d", main_module_index, submodule_index);
    else snprintf(buf, n, "%d", g_zenv.ppu ? g_zenv.ppu->extraLeftRight : 0);
    break;
  case kSid_CheatLife:
  case kSid_CheatKeys:
  case kSid_CheatGear:
    snprintf(buf, n, "A");
    break;
  default:
    buf[0] = 0;
    break;
  }
}

static void Set_Adjust(const SetRow *r, int dir) {
  bool *bp;
  if (r->flags & kRowF_Info)
    return;
  if (r->sid == kSid_CheatLife) { PatchCommand('w'); return; }
  if (r->sid == kSid_CheatKeys) { PatchCommand('o'); return; }
  if (r->sid == kSid_CheatGear) { PatchCommand('W'); return; }

  switch (r->sid) {
  case kSid_FeatureBit:
    g_config.features0 ^= r->extra;
    break;
  case kSid_Bool: {
    int aspect_idx = Set_AspectIndex();
    bp = Set_BoolPtr(r->extra);
    if (bp) *bp = !*bp;
    if (r->extra == 2)
      Set_SetAspect(aspect_idx);
    break;
  }
  case kSid_Aspect:
    Set_SetAspect((Set_AspectIndex() + (dir < 0 ? 2 : 1)) % 3);
    break;
  case kSid_Fullscreen:
    g_config.fullscreen = (uint8)((g_config.fullscreen + (dir < 0 ? 2 : 1)) % 3);
    break;
  case kSid_WindowScale: {
    int s = g_config.window_scale ? g_config.window_scale : 2;
    s += (dir < 0) ? -1 : 1;
    if (s < 1) s = 1;
    if (s > 10) s = 10;
    g_config.window_scale = (uint8)s;
    break;
  }
  case kSid_OutputMethod:
    g_config.output_method = (uint8)((g_config.output_method + (dir < 0 ? 3 : 1)) % 4);
    break;
  case kSid_AudioFreq: {
    static const uint16 freqs[] = { 11025, 22050, 32000, 44100, 48000 };
    int i = 3;
    for (int j = 0; j < 5; j++)
      if (freqs[j] == g_config.audio_freq) i = j;
    i += (dir < 0) ? -1 : 1;
    if (i < 0) i = 0;
    if (i > 4) i = 4;
    g_config.audio_freq = freqs[i];
    break;
  }
  case kSid_AudioSamples: {
    static const uint16 smp[] = { 256, 512, 1024, 2048, 4096 };
    int i = 1;
    for (int j = 0; j < 5; j++)
      if (smp[j] == g_config.audio_samples) i = j;
    i += (dir < 0) ? -1 : 1;
    if (i < 0) i = 0;
    if (i > 4) i = 4;
    g_config.audio_samples = smp[i];
    break;
  }
  case kSid_AudioCh:
    g_config.audio_channels = (g_config.audio_channels == 1) ? 2 : 1;
    break;
  case kSid_MsuMode: {
    static const uint8 modes[] = { 0, kMsuEnabled_Msu, kMsuEnabled_MsuDeluxe, kMsuEnabled_Opuz, kMsuEnabled_MsuDeluxe | kMsuEnabled_Opuz };
    int i = 0;
    for (int j = 0; j < 5; j++)
      if (modes[j] == g_config.enable_msu) i = j;
    i += (dir < 0) ? -1 : 1;
    if (i < 0) i = 0;
    if (i > 4) i = 4;
    g_config.enable_msu = modes[i];
    if (g_config.enable_msu & kMsuEnabled_Opuz)
      g_config.audio_freq = 48000;
    else if (g_config.enable_msu)
      g_config.audio_freq = 44100;
    break;
  }
  case kSid_MsuVol: {
    int v = g_config.msuvolume + (dir < 0 ? -5 : 5);
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    g_config.msuvolume = (uint8)v;
    break;
  }
  case kSid_DevOverlay: {
    int i = 0;
    for (int j = 0; j < 6; j++)
      if (kForceOverlayIds[j] == g_dev_force_overlay) i = j;
    i += (dir < 0) ? -1 : 1;
    if (i < 0) i = 0;
    if (i > 5) i = 5;
    g_dev_force_overlay = kForceOverlayIds[i];
    Set_ApplyOverlay();
    break;
  }
  case kSid_Language:
    Set_CycleLanguage(dir);
    break;
  default:
    return;
  }
  Set_MarkDirty();
  if (r->sid == kSid_AudioFreq || r->sid == kSid_AudioSamples ||
      r->sid == kSid_AudioCh || r->sid == kSid_MsuMode ||
      (r->sid == kSid_Bool && r->extra == 11))
    Settings_ApplyAudio();
}

void SettingsMenu_Open(int from) {
  g_ui = kSetUi_Menu;
  g_from = from;
  g_tab = 0;
  g_row = 0;
  g_scroll = 0;
  g_help_tab = -1;
  g_help_row = -1;
  g_help_tick = 0;
  sound_effect_2 = 0x20;
}

void SettingsMenu_Close(void) {
  if (g_dirty) {
    WriteUserConfigFile();
    g_dirty = false;
  }
  if (g_from == kSettingsFrom_Pause)
    g_ui = kSetUi_PauseChooser;
  else
    g_ui = kSetUi_Closed;
  sound_effect_2 = 0x18;
}

bool SettingsMenu_IsOpen(void) {
  return g_ui == kSetUi_Menu;
}

void SettingsPauseChooser_Reset(void) {
  g_ui = kSetUi_PauseChooser;
  g_chooser_row = 0;
}

bool SettingsPauseChooser_IsActive(void) {
  return g_ui == kSetUi_PauseChooser;
}

static void Set_ClampRow(void) {
  int count;
  Set_TabRows(g_tab, &count);
  if (g_row < 0) g_row = 0;
  if (g_row >= count) g_row = count - 1;
  if (g_row < g_scroll) g_scroll = g_row;
  if (g_row >= g_scroll + 10) g_scroll = g_row - 9;
}

void SettingsMenu_Run(void) {
  int count, ntab = Set_VisibleTabCount();
  const SetRow *rows;
  uint8 h = filtered_joypad_H;
  uint8 l = filtered_joypad_L;

  if (g_tab >= ntab)
    g_tab = 0;
  if (h & kJoypadH_B || h & kJoypadH_Start) {
    SettingsMenu_Close();
    filtered_joypad_H = 0;
    filtered_joypad_L = 0;
    return;
  }
  if (l & kJoypadL_L) {
    if (--g_tab < 0) g_tab = ntab - 1;
    g_row = g_scroll = 0;
    sound_effect_2 = 0x20;
  } else if ((l & kJoypadL_R) || (h & kJoypadH_Select)) {
    if (++g_tab >= ntab) g_tab = 0;
    g_row = g_scroll = 0;
    sound_effect_2 = 0x20;
  }
  rows = Set_TabRows(g_tab, &count);
  if (h & kJoypadH_Up) {
    if (--g_row < 0) g_row = count - 1;
    sound_effect_2 = 0x20;
  } else if (h & kJoypadH_Down) {
    if (++g_row >= count) g_row = 0;
    sound_effect_2 = 0x20;
  } else if (h & kJoypadH_Left) {
    Set_Adjust(&rows[g_row], -1);
    sound_effect_2 = 0x2c;
  } else if (h & kJoypadH_Right || l & kJoypadL_A) {
    Set_Adjust(&rows[g_row], 1);
    sound_effect_2 = 0x2c;
  }
  Set_ClampRow();
  filtered_joypad_H = 0;
  filtered_joypad_L = 0;
}

void SettingsPauseChooser_Run(void) {
  if (g_ui == kSetUi_Menu) {
    SettingsMenu_Run();
    return;
  }
  if (g_ui != kSetUi_PauseChooser)
    SettingsPauseChooser_Reset();

  uint8 h = filtered_joypad_H;
  uint8 l = filtered_joypad_L;
  if (h & kJoypadH_Up) {
    if (--g_chooser_row < 0) g_chooser_row = 2;
    sound_effect_2 = 0x20;
  } else if (h & kJoypadH_Down) {
    if (++g_chooser_row > 2) g_chooser_row = 0;
    sound_effect_2 = 0x20;
  } else if (h & kJoypadH_B) {
    Set_ReturnToGameplay();
    sound_effect_2 = 0x18;
  } else if ((l & kJoypadL_A) || (h & kJoypadH_Start)) {
    if (g_chooser_row == 0) {
      Set_ReturnToGameplay();
      sound_effect_2 = 0x2c;
    } else if (g_chooser_row == 1) {
      SettingsMenu_Open(kSettingsFrom_Pause);
    } else {
      g_ui = kSetUi_Closed;
      sound_effect_ambient = 15;
      main_module_index = 23;
      submodule_index = 1;
      index_of_changable_dungeon_objs[0] = 0;
      index_of_changable_dungeon_objs[1] = 0;
    }
  }
  filtered_joypad_H = 0;
  filtered_joypad_L = 0;
}

/* 8x8 caps font, public-domain style. Rows are bit7 = leftmost. */
static const uint8 kFont[96][8] = {
  {0,0,0,0,0,0,0,0},
  {0x18,0x18,0x18,0x18,0x18,0,0x18,0},
  {0x6c,0x6c,0x24,0,0,0,0,0},
  {0x6c,0xfe,0x6c,0x6c,0xfe,0x6c,0,0},
  {0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0},
  {0x62,0x66,0x0c,0x18,0x30,0x66,0x46,0},
  {0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0},
  {0x18,0x18,0x30,0,0,0,0,0},
  {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0},
  {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0},
  {0,0x66,0x3c,0xff,0x3c,0x66,0,0},
  {0,0x18,0x18,0x7e,0x18,0x18,0,0},
  {0,0,0,0,0,0x18,0x18,0x30},
  {0,0,0,0x7e,0,0,0,0},
  {0,0,0,0,0,0x18,0x18,0},
  {0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0},
  {0x3c,0x66,0x6e,0x76,0x66,0x66,0x3c,0},
  {0x18,0x38,0x18,0x18,0x18,0x18,0x7e,0},
  {0x3c,0x66,0x06,0x1c,0x30,0x60,0x7e,0},
  {0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0},
  {0x0c,0x1c,0x3c,0x6c,0x7e,0x0c,0x0c,0},
  {0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0},
  {0x1c,0x30,0x60,0x7c,0x66,0x66,0x3c,0},
  {0x7e,0x06,0x0c,0x18,0x30,0x30,0x30,0},
  {0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0},
  {0x3c,0x66,0x66,0x3e,0x06,0x0c,0x38,0},
  {0,0x18,0x18,0,0,0x18,0x18,0},
  {0,0x18,0x18,0,0,0x18,0x18,0x30},
  {0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0},
  {0,0,0x7e,0,0x7e,0,0,0},
  {0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0},
  {0x3c,0x66,0x06,0x0c,0x18,0,0x18,0},
  {0x3c,0x66,0x6e,0x6a,0x6e,0x60,0x3c,0},
  {0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0},
  {0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0},
  {0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0},
  {0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0},
  {0x7e,0x60,0x60,0x7c,0x60,0x60,0x7e,0},
  {0x7e,0x60,0x60,0x7c,0x60,0x60,0x60,0},
  {0x3c,0x66,0x60,0x6e,0x66,0x66,0x3c,0},
  {0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0},
  {0x7e,0x18,0x18,0x18,0x18,0x18,0x7e,0},
  {0x3e,0x0c,0x0c,0x0c,0x0c,0x6c,0x38,0},
  {0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0},
  {0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0},
  {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0},
  {0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0},
  {0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0},
  {0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0},
  {0x3c,0x66,0x66,0x66,0x6a,0x6c,0x36,0},
  {0x7c,0x66,0x66,0x7c,0x6c,0x66,0x66,0},
  {0x3c,0x66,0x60,0x3c,0x06,0x66,0x3c,0},
  {0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0},
  {0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0},
  {0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0},
  {0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0},
  {0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0},
  {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0},
  {0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0},
  {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0},
  {0xc0,0x60,0x30,0x18,0x0c,0x06,0x02,0},
  {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0},
  {0x18,0x3c,0x66,0,0,0,0,0},
  {0,0,0,0,0,0,0,0xff},
  {0x18,0x18,0x0c,0,0,0,0,0},
  {0,0,0x3c,0x06,0x3e,0x66,0x3e,0},
  {0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0},
  {0,0,0x3c,0x66,0x60,0x66,0x3c,0},
  {0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0},
  {0,0,0x3c,0x66,0x7e,0x60,0x3c,0},
  {0x1c,0x30,0x7c,0x30,0x30,0x30,0x30,0},
  {0,0,0x3e,0x66,0x66,0x3e,0x06,0x3c},
  {0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0},
  {0x18,0,0x38,0x18,0x18,0x18,0x3c,0},
  {0x0c,0,0x1c,0x0c,0x0c,0x0c,0x6c,0x38},
  {0x60,0x60,0x66,0x6c,0x78,0x6c,0x66,0},
  {0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0},
  {0,0,0x66,0x7f,0x7f,0x6b,0x63,0},
  {0,0,0x7c,0x66,0x66,0x66,0x66,0},
  {0,0,0x3c,0x66,0x66,0x66,0x3c,0},
  {0,0,0x7c,0x66,0x66,0x7c,0x60,0x60},
  {0,0,0x3e,0x66,0x66,0x3e,0x06,0x06},
  {0,0,0x6c,0x76,0x60,0x60,0x60,0},
  {0,0,0x3e,0x60,0x3c,0x06,0x7c,0},
  {0x30,0x30,0x7c,0x30,0x30,0x30,0x1c,0},
  {0,0,0x66,0x66,0x66,0x66,0x3e,0},
  {0,0,0x66,0x66,0x66,0x3c,0x18,0},
  {0,0,0x63,0x6b,0x7f,0x3e,0x36,0},
  {0,0,0x66,0x3c,0x18,0x3c,0x66,0},
  {0,0,0x66,0x66,0x66,0x3e,0x06,0x3c},
  {0,0,0x7e,0x0c,0x18,0x30,0x7e,0},
  {0x0e,0x18,0x18,0x70,0x18,0x18,0x0e,0},
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0},
  {0x70,0x18,0x18,0x0e,0x18,0x18,0x70,0},
  {0x76,0xdc,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
};

#define COL_RGB(r,g,b) ((uint32)(r)<<16 | (uint32)(g)<<8 | (uint32)(b))

static void Pix_Set(uint8 *pixel_buffer, size_t pitch, int x, int y, uint32 c) {
  *(uint32 *)(pixel_buffer + (size_t)y * pitch + (size_t)x * 4) = c;
}

static void Pix_Fill(uint8 *pb, size_t pitch, int x0, int y0, int x1, int y1, uint32 c) {
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++)
      Pix_Set(pb, pitch, x, y, c);
}

static void Pix_Darken(uint8 *pb, size_t pitch, int width, int height) {
  for (int y = 0; y < height; y++) {
    uint32 *row = (uint32 *)(pb + (size_t)y * pitch);
    for (int x = 0; x < width; x++) {
      uint32 p = row[x];
      row[x] = ((p >> 2) & 0x3f3f3f) + ((p >> 3) & 0x1f1f1f);
    }
  }
}

static void Pix_Char(uint8 *pb, size_t pitch, int x, int y, char ch, uint32 c) {
  unsigned uc = (unsigned char)ch;
  if (uc < 32 || uc > 127) uc = '?';
  const uint8 *g = kFont[uc - 32];
  for (int row = 0; row < 8; row++) {
    uint8 bits = g[row];
    for (int col = 0; col < 8; col++) {
      if (bits & (0x80 >> col))
        Pix_Set(pb, pitch, x + col, y + row, c);
    }
  }
}

static void Pix_Text(uint8 *pb, size_t pitch, int x, int y, const char *s, uint32 c) {
  for (; *s; s++) {
    Pix_Char(pb, pitch, x, y, *s, c);
    x += 8;
  }
}

static void Pix_CharClip(uint8 *pb, size_t pitch, int x, int y, char ch, uint32 c, int clip_x0, int clip_x1) {
  unsigned uc = (unsigned char)ch;
  if (uc < 32 || uc > 127) uc = '?';
  const uint8 *g = kFont[uc - 32];
  for (int row = 0; row < 8; row++) {
    uint8 bits = g[row];
    for (int col = 0; col < 8; col++) {
      int px = x + col;
      if ((bits & (0x80 >> col)) && px >= clip_x0 && px < clip_x1)
        Pix_Set(pb, pitch, px, y + row, c);
    }
  }
}

static void Pix_TextClip(uint8 *pb, size_t pitch, int x, int y, const char *s, uint32 c, int clip_x0, int clip_x1) {
  for (; *s; s++) {
    if (x + 8 > clip_x0 && x < clip_x1)
      Pix_CharClip(pb, pitch, x, y, *s, c, clip_x0, clip_x1);
    x += 8;
  }
}

static int Help_ScrollOffset(int overflow) {
  enum { kHold = 48, kDiv = 2 };
  if (overflow <= 0)
    return 0;
  int travel = overflow * kDiv;
  int period = (travel + kHold) * 2;
  int t = g_help_tick % period;
  if (t < kHold)
    return 0;
  t -= kHold;
  if (t < travel)
    return t / kDiv;
  t -= travel;
  if (t < kHold)
    return overflow;
  t -= kHold;
  return overflow - t / kDiv;
}

static void Pix_Rect(uint8 *pb, size_t pitch, int x0, int y0, int x1, int y1, uint32 c) {
  for (int x = x0; x < x1; x++) {
    Pix_Set(pb, pitch, x, y0, c);
    Pix_Set(pb, pitch, x, y1 - 1, c);
  }
  for (int y = y0; y < y1; y++) {
    Pix_Set(pb, pitch, x0, y, c);
    Pix_Set(pb, pitch, x1 - 1, y, c);
  }
}

static void Draw_Window(uint8 *pb, size_t pitch, int x0, int y0, int x1, int y1) {
  Pix_Fill(pb, pitch, x0, y0, x1, y1, COL_RGB(16, 24, 56));
  Pix_Rect(pb, pitch, x0, y0, x1, y1, COL_RGB(232, 208, 160));
  Pix_Rect(pb, pitch, x0 + 1, y0 + 1, x1 - 1, y1 - 1, COL_RGB(136, 104, 32));
}

static void Draw_Fairy(uint8 *pb, size_t pitch, int x, int y) {
  uint32 c = (frame_counter & 8) ? COL_RGB(248, 80, 168) : COL_RGB(248, 168, 216);
  static const char *rows[7] = {
    "  ##  ",
    " #### ",
    "######",
    "######",
    " #### ",
    "  ##  ",
    " #  # ",
  };
  for (int r = 0; r < 7; r++) {
    for (int col = 0; col < 6; col++) {
      if (rows[r][col] == '#')
        Pix_Set(pb, pitch, x + col, y + r, c);
    }
  }
}

static void Draw_FileSelectHint(uint8 *pb, size_t pitch, int extra, int height) {
  (void)height;
  int x = extra + 72;
  Pix_Text(pb, pitch, x, 208, "START: OPTIONS", COL_RGB(248, 240, 32));
}

static void Draw_Chooser(uint8 *pb, size_t pitch, int extra, int height) {
  int w = 256 + extra * 2;
  Pix_Darken(pb, pitch, w, height);
  int x0 = extra + 40, y0 = 64, x1 = extra + 216, y1 = 160;
  Draw_Window(pb, pitch, x0, y0, x1, y1);
  Pix_Text(pb, pitch, extra + 88, 72, "PAUSE", COL_RGB(248, 240, 32));
  static const char *lines[] = { "CONTINUE", "OPTIONS", "SAVE AND QUIT" };
  for (int i = 0; i < 3; i++) {
    int y = 92 + i * 16;
    if (i == g_chooser_row)
      Draw_Fairy(pb, pitch, extra + 52, y);
    Pix_Text(pb, pitch, extra + 68, y, lines[i],
             i == g_chooser_row ? COL_RGB(248, 240, 32) : COL_RGB(240, 240, 240));
  }
}

static void Draw_Menu(uint8 *pb, size_t pitch, int extra, int height) {
  int w = 256 + extra * 2;
  Pix_Darken(pb, pitch, w, height);
  int x0 = extra + 8, y0 = 8, x1 = extra + 248, y1 = height - 8;
  Draw_Window(pb, pitch, x0, y0, x1, y1);

  int tx = extra + 16;
  for (int t = 0; t < Set_VisibleTabCount(); t++) {
    uint32 c = (t == g_tab) ? COL_RGB(248, 240, 32) : COL_RGB(160, 160, 176);
    Pix_Text(pb, pitch, tx, 14, kTabNames[t], c);
    tx += (int)strlen(kTabNames[t]) * 8 + 8;
  }

  int count;
  const SetRow *rows = Set_TabRows(g_tab, &count);
  char val[48];
  for (int vis = 0; vis < 10; vis++) {
    int i = g_scroll + vis;
    if (i >= count) break;
    int y = 32 + vis * 14;
    if (i == g_row)
      Draw_Fairy(pb, pitch, extra + 14, y);
    uint32 lc = (i == g_row) ? COL_RGB(248, 240, 32) : COL_RGB(240, 240, 240);
    Pix_Text(pb, pitch, extra + 24, y, rows[i].label, lc);
    Set_Format(&rows[i], val, sizeof(val));
    if (val[0]) {
      int vx = extra + 248 - 8 - (int)strlen(val) * 8;
      if (vx < extra + 140) vx = extra + 140;
      uint32 vc = COL_RGB(72, 224, 72);
      if (!strcmp(val, "OFF")) vc = COL_RGB(224, 64, 64);
      if (rows[i].flags & kRowF_Info) vc = COL_RGB(160, 176, 208);
      Pix_Text(pb, pitch, vx, y, val, vc);
    }
  }

  const char *help = rows[g_row].help;
  int hx0 = extra + 16, hx1 = extra + 240;
  Pix_Fill(pb, pitch, extra + 12, height - 28, extra + 244, height - 12, COL_RGB(8, 12, 32));
  if (help) {
    if (g_help_tab != g_tab || g_help_row != g_row) {
      g_help_tab = g_tab;
      g_help_row = g_row;
      g_help_tick = 0;
    } else {
      g_help_tick++;
    }
    int text_w = (int)strlen(help) * 8;
    int vis = hx1 - hx0;
    int off = Help_ScrollOffset(text_w - vis);
    Pix_TextClip(pb, pitch, hx0 - off, height - 24, help, COL_RGB(192, 192, 128), hx0, hx1);
  }
}

void SettingsMenu_Draw(uint8 *pixel_buffer, size_t pitch, int height) {
  if (!pixel_buffer)
    return;
  Ppu *ppu = g_zenv.ppu;
  int extra = ppu ? ppu->extraLeftRight : 0;

  if (g_ui == kSetUi_Menu)
    Draw_Menu(pixel_buffer, pitch, extra, height);
  else if (g_ui == kSetUi_PauseChooser)
    Draw_Chooser(pixel_buffer, pitch, extra, height);
  else if (main_module_index == 1 && submodule_index == 5)
    Draw_FileSelectHint(pixel_buffer, pitch, extra, height);
}
