#include "extract.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#else
#include <sys/wait.h>
#endif

#include <SDL.h>

#include "types.h"
#include "assets.h"
#include "util.h"

#define kAssetsPath "zelda3_assets.dat"
#define kAssetsPathTmp "zelda3_assets.dat.tmp"

static const char kUsRomSha1[] = "6D4F10A8B10E10DBE624CB23CF03B88BB8252973";

static const uint32 kCompSpritePtrs[108] = {
  0x10f000, 0x10f600, 0x10fc00, 0x118200, 0x118800, 0x118e00, 0x119400, 0x119a00,
  0x11a000, 0x11a600, 0x11ac00, 0x11b200, 0x14fffc, 0x1585d4, 0x158ab6, 0x158fbe,
  0x1593f8, 0x1599a6, 0x159f32, 0x15a3d7, 0x15a8f1, 0x15aec6, 0x15b418, 0x15b947,
  0x15bed0, 0x15c449, 0x15c975, 0x15ce7c, 0x15d394, 0x15d8ac, 0x15ddc0, 0x15e34c,
  0x15e8e8, 0x15ee31, 0x15f3a6, 0x15f92d, 0x15feba, 0x1682ff, 0x1688e0, 0x168e41,
  0x1692df, 0x169883, 0x169cd0, 0x16a26e, 0x16a275, 0x16a787, 0x16aa06, 0x16ae9d,
  0x16b3ff, 0x16b87e, 0x16be6b, 0x16c13d, 0x16c619, 0x16cbbb, 0x16d0f1, 0x16d641,
  0x16d95a, 0x16dd99, 0x16e278, 0x16e760, 0x16ed25, 0x16f20f, 0x16f6b7, 0x16fa5f,
  0x16fd29, 0x1781cd, 0x17868d, 0x178b62, 0x178fd5, 0x179527, 0x17994b, 0x179ea7,
  0x17a30e, 0x17a805, 0x17acf8, 0x17b2a2, 0x17b7f9, 0x17bc93, 0x17c237, 0x17c78e,
  0x17cd55, 0x17d2bc, 0x17d82f, 0x17dcec, 0x17e1cc, 0x17e36b, 0x17e842, 0x17eb38,
  0x17ed58, 0x17f06c, 0x17f4fd, 0x17fa39, 0x17ff86, 0x18845c, 0x1889a1, 0x188d64,
  0x18919d, 0x189610, 0x189857, 0x189b24, 0x189dd2, 0x18a03f, 0x18a4ed, 0x18a7ba,
  0x18aedf, 0x18af0d, 0x18b520, 0x18b953,
};

static const uint32 kCompBgPtrs[115] = {
  0x11b800, 0x11bce2, 0x11c15f, 0x11c675, 0x11cb84, 0x11cf4c, 0x11d2ce, 0x11d726,
  0x11d9cf, 0x11dec4, 0x11e393, 0x11e893, 0x11ed7d, 0x11f283, 0x11f746, 0x11fc21,
  0x11fff2, 0x128498, 0x128a0e, 0x128f30, 0x129326, 0x129804, 0x129d5b, 0x12a272,
  0x12a6fe, 0x12aa77, 0x12ad83, 0x12b167, 0x12b51d, 0x12b840, 0x12bd54, 0x12c1c9,
  0x12c73d, 0x12cc86, 0x12d198, 0x12d6b1, 0x12db6a, 0x12e0ea, 0x12e6bd, 0x12eb51,
  0x12f135, 0x12f6c5, 0x12fc71, 0x138129, 0x138693, 0x138bad, 0x139117, 0x139609,
  0x139b21, 0x13a074, 0x13a619, 0x13ab2b, 0x13b00c, 0x13b4f5, 0x13b9eb, 0x13bebf,
  0x13c3ce, 0x13c817, 0x13cb68, 0x13cfb5, 0x13d460, 0x13d8c2, 0x13dd7a, 0x13e266,
  0x13e7af, 0x13ece5, 0x13f245, 0x13f6f0, 0x13fc30, 0x1480e9, 0x14863b, 0x148a7c,
  0x148f2a, 0x149346, 0x1497ed, 0x149cc2, 0x14a173, 0x14a61d, 0x14ab5d, 0x14b083,
  0x14b4bd, 0x14b94e, 0x14be0e, 0x14c291, 0x14c7ba, 0x14cce4, 0x14d1db, 0x14d6bd,
  0x14db77, 0x14ded1, 0x14e2ac, 0x14e754, 0x14ebae, 0x14ef4e, 0x14f309, 0x14f6f4,
  0x14fa55, 0x14ff8c, 0x14ff93, 0x14ff9a, 0x14ffa1, 0x14ffa8, 0x14ffaf, 0x14ffb6,
  0x14ffbd, 0x14ffc4, 0x14ffcb, 0x14ffd2, 0x14ffd9, 0x14ffe0, 0x14ffe7, 0x14ffee,
  0x14fff5, 0x18b520, 0x18b953,
};

static const char *const kTextDictionaryUS[] = {
  "    ", "   ", "  ", "'s ", "and ", "are ", "all ", "ain", "and", "at ",
  "ast", "an", "at", "ble", "ba", "be", "bo", "can ", "che", "com",
  "ck", "des", "di", "do", "en ", "er ", "ear", "ent", "ed ", "en",
  "er", "ev", "for", "fro", "give ", "get", "go", "have", "has", "her",
  "hi", "ha", "ight ", "ing ", "in", "is", "it", "just", "know", "ly ",
  "la", "lo", "man", "ma", "me", "mu", "n't ", "non", "not", "open",
  "ound", "out ", "of", "on", "or", "per", "ple", "pow", "pro", "re ",
  "re", "some", "se", "sh", "so", "st", "ter ", "thin", "ter", "tha",
  "the", "thi", "to", "tr", "up", "ver", "with", "wa", "we", "wh",
  "wi", "you", "Her", "Tha", "The", "Thi", "You",
};

static const char *const kTextAlphabetUS[] = {
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
  "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "a", "b", "c", "d", "e", "f",
  "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v",
  "w", "x", "y", "z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "!", "?",
  "-", ".", ",", "[...]", ">", "(", ")",
  "[Ankh]", "[Waves]", "[Snake]", "[LinkL]", "[LinkR]",
  "\"", "[Up]", "[Down]", "[Left]",
  "[Right]", "'", "[1HeartL]", "[1HeartR]", "[2HeartL]", "[3HeartL]", "[3HeartR]",
  "[4HeartL]", "[4HeartR]", " ", "<", "[A]", "[B]", "[X]", "[Y]",
};

static const uint8 kTextCmdLenUS[25] = {
  1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1,
};

static const uint8 kExtraDialogue[] = {
  0x7a, 0x00, 0x34, 0x40, 0x59, 0x6c, 0x00, 0x41, 0x59, 0x35, 0x40, 0x59, 0x6c, 0x01,
  0x75, 0x36, 0x40, 0x59, 0x6c, 0x02, 0x41, 0x59, 0x37, 0x40, 0x59, 0x6c, 0x03,
};

static const uint8 kAssetsSigBytes[] = { kAssets_Sig };

// ---------------------------------------------------------------------------
// SHA-1
// ---------------------------------------------------------------------------

typedef struct {
  uint32 state[5];
  uint32 count[2];
  uint8 buffer[64];
} Sha1Ctx;

static uint32 Sha1Rol(uint32 v, int n) { return (v << n) | (v >> (32 - n)); }

static void Sha1Transform(uint32 st[5], const uint8 buf[64]) {
  uint32 w[80];
  for (int i = 0; i < 16; i++)
    w[i] = ((uint32)buf[i * 4] << 24) | ((uint32)buf[i * 4 + 1] << 16) |
           ((uint32)buf[i * 4 + 2] << 8) | buf[i * 4 + 3];
  for (int i = 16; i < 80; i++)
    w[i] = Sha1Rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  uint32 a = st[0], b = st[1], c = st[2], d = st[3], e = st[4];
  for (int i = 0; i < 80; i++) {
    uint32 f, k;
    if (i < 20) {
      f = (b & c) | ((~b) & d);
      k = 0x5A827999;
    } else if (i < 40) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1;
    } else if (i < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDC;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6;
    }
    uint32 t = Sha1Rol(a, 5) + f + e + k + w[i];
    e = d;
    d = c;
    c = Sha1Rol(b, 30);
    b = a;
    a = t;
  }
  st[0] += a;
  st[1] += b;
  st[2] += c;
  st[3] += d;
  st[4] += e;
}

static void Sha1Init(Sha1Ctx *c) {
  c->state[0] = 0x67452301;
  c->state[1] = 0xEFCDAB89;
  c->state[2] = 0x98BADCFE;
  c->state[3] = 0x10325476;
  c->state[4] = 0xC3D2E1F0;
  c->count[0] = c->count[1] = 0;
}

static void Sha1Update(Sha1Ctx *c, const uint8 *data, size_t len) {
  uint32 i = (c->count[0] >> 3) & 63;
  uint32 nbits = (uint32)(len << 3);
  c->count[0] += nbits;
  if (c->count[0] < nbits)
    c->count[1]++;
  c->count[1] += (uint32)(len >> 29);
  uint32 part = 64 - i;
  size_t off = 0;
  if (len >= part) {
    memcpy(c->buffer + i, data, part);
    Sha1Transform(c->state, c->buffer);
    for (off = part; off + 63 < len; off += 64)
      Sha1Transform(c->state, data + off);
    i = 0;
  }
  memcpy(c->buffer + i, data + off, len - off);
}

static void Sha1Final(Sha1Ctx *c, uint8 digest[20]) {
  uint8 finalcount[8];
  for (int i = 0; i < 8; i++)
    finalcount[i] = (uint8)((c->count[(i >= 4) ? 0 : 1] >> ((3 - (i & 3)) * 8)) & 255);
  uint8 one = 0x80;
  Sha1Update(c, &one, 1);
  uint8 z = 0;
  while ((c->count[0] & 504) != 448)
    Sha1Update(c, &z, 1);
  Sha1Update(c, finalcount, 8);
  for (int i = 0; i < 20; i++)
    digest[i] = (uint8)((c->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
}

static void Sha1Hex(const uint8 *data, size_t len, char out[41]) {
  Sha1Ctx c;
  uint8 d[20];
  Sha1Init(&c);
  Sha1Update(&c, data, len);
  Sha1Final(&c, d);
  static const char hex[] = "0123456789ABCDEF";
  for (int i = 0; i < 20; i++) {
    out[i * 2] = hex[d[i] >> 4];
    out[i * 2 + 1] = hex[d[i] & 0xf];
  }
  out[40] = 0;
}

// ---------------------------------------------------------------------------
// ROM
// ---------------------------------------------------------------------------

static uint8 *g_rom;
static size_t g_rom_size;

static uint32 RomMap(uint32 ea) {
  return ((ea >> 16) & 0x7f) * 0x8000 + (ea & 0x7fff);
}

static uint8 RomByte(uint32 ea) {
  uint32 off = RomMap(ea);
  if (off >= g_rom_size)
    Die("ROM address out of range during extract");
  return g_rom[off];
}

static uint16 RomWord(uint32 ea) {
  return (uint16)(RomByte(ea) | (RomByte(ea + 1) << 8));
}

static uint32 Rom24(uint32 ea) {
  return (uint32)RomByte(ea) | ((uint32)RomByte(ea + 1) << 8) | ((uint32)RomByte(ea + 2) << 16);
}

static int8 RomInt8(uint32 ea) {
  uint8 b = RomByte(ea);
  return (int8)b;
}

static int16 RomInt16(uint32 ea) {
  uint16 w = RomWord(ea);
  return (int16)w;
}

static uint32 RomStep(uint32 ea) {
  ea++;
  if ((ea & 0x8000) == 0)
    ea += 0x8000;
  return ea;
}

static void RomAppend(ByteArray *a, uint32 ea, int n) {
  for (int i = 0; i < n; i++) {
    ByteArray_AppendByte(a, RomByte(ea));
    ea = RomStep(ea);
  }
}

typedef struct {
  uint32 ea;
} RomReader;

static uint8 ReaderNext(RomReader *r) {
  uint8 b = RomByte(r->ea);
  r->ea++;
  if ((r->ea & 0xffff) == 0)
    r->ea += 0x8000;
  return b;
}

static void RomDecomp(uint32 ea, bool offset_is_be, ByteArray *out, int *comp_len) {
  RomReader r = { ea };
  for (;;) {
    uint8 b = ReaderNext(&r);
    if (b == 0xff) {
      if (comp_len)
        *comp_len = (int)((r.ea - ea) & 0x7fff);
      return;
    }
    int lx, cmd;
    if ((b & 0xe0) != 0xe0) {
      lx = b & 0x1f;
      cmd = b & 0xe0;
    } else {
      cmd = (b << 3) & 0xe0;
      lx = ((b & 3) << 8) | ReaderNext(&r);
    }
    lx++;
    if (cmd == 0x00) {
      while (lx--)
        ByteArray_AppendByte(out, ReaderNext(&r));
    } else if (cmd & 0x80) {
      uint32 offs = ((uint32)ReaderNext(&r) << 8) | ReaderNext(&r);
      if (!offset_is_be)
        offs = ((offs >> 8) | (offs << 8)) & 0xffff;
      while (lx--) {
        if (offs >= out->size)
          Die("Invalid compression offset in ROM");
        ByteArray_AppendByte(out, out->data[offs++]);
      }
    } else if ((cmd & 0x40) == 0) {
      uint8 v = ReaderNext(&r);
      while (lx--)
        ByteArray_AppendByte(out, v);
    } else if ((cmd & 0x20) == 0) {
      uint8 b1 = ReaderNext(&r), b2 = ReaderNext(&r);
      while (lx) {
        ByteArray_AppendByte(out, b1);
        if (lx == 1)
          break;
        ByteArray_AppendByte(out, b2);
        lx -= 2;
      }
    } else {
      uint8 v = ReaderNext(&r);
      while (lx--) {
        ByteArray_AppendByte(out, v);
        v = (uint8)(v + 1);
      }
    }
  }
}

static int RomDecompLen(uint32 ea, bool offset_is_be) {
  ByteArray tmp = { 0 };
  int n = 0;
  RomDecomp(ea, offset_is_be, &tmp, &n);
  ByteArray_Destroy(&tmp);
  return n;
}

// ---------------------------------------------------------------------------
// Asset list / packing
// ---------------------------------------------------------------------------

typedef struct {
  const char *name;
  ByteArray data;
} PackedAsset;

static PackedAsset g_assets[kNumberOfAssets];
static int g_nassets;

static void BA_Clear(ByteArray *a) {
  ByteArray_Destroy(a);
  memset(a, 0, sizeof(*a));
}

static void AppendU16(ByteArray *a, uint16 v) {
  ByteArray_AppendByte(a, (uint8)v);
  ByteArray_AppendByte(a, (uint8)(v >> 8));
}

static void AppendU32(ByteArray *a, uint32 v) {
  ByteArray_AppendByte(a, (uint8)v);
  ByteArray_AppendByte(a, (uint8)(v >> 8));
  ByteArray_AppendByte(a, (uint8)(v >> 16));
  ByteArray_AppendByte(a, (uint8)(v >> 24));
}

static void PackArrays(ByteArray *out, ByteArray *parts, int n) {
  if (n <= 0)
    return;
  size_t offs = 0;
  for (int i = 0; i < n - 1; i++)
    offs += parts[i].size;
  int use32 = (offs >= 65536 || n > 8192);
  offs = 0;
  for (int i = 0; i < n - 1; i++) {
    offs += parts[i].size;
    if (use32)
      AppendU32(out, (uint32)offs);
    else
      AppendU16(out, (uint16)offs);
  }
  for (int i = 0; i < n; i++)
    ByteArray_AppendData(out, parts[i].data, parts[i].size);
  AppendU16(out, (uint16)(use32 ? 8192 + n - 1 : n - 1));
}

static void AddAsset(const char *name, ByteArray *data) {
  if (g_nassets >= kNumberOfAssets)
    Die("Too many extracted assets");
  char *copy = (char *)malloc(strlen(name) + 1);
  if (!copy)
    Die("Out of memory");
  memcpy(copy, name, strlen(name) + 1);
  g_assets[g_nassets].name = copy;
  g_assets[g_nassets].data = *data;
  memset(data, 0, sizeof(*data));
  g_nassets++;
}

static void AddRomBytes(const char *name, uint32 ea, int n) {
  ByteArray a = { 0 };
  RomAppend(&a, ea, n);
  AddAsset(name, &a);
}

static void AddRomWords(const char *name, uint32 ea, int n) {
  AddRomBytes(name, ea, n * 2);
}

static size_t AppendScanBytes(ByteArray *big, const uint8 *little, int n) {
  for (int keep = n; keep >= 0; keep--) {
    int ok = 1;
    if (keep > 0) {
      if (big->size < (size_t)keep)
        continue;
      if (memcmp(big->data + big->size - keep, little, (size_t)keep) != 0)
        ok = 0;
    }
    if (ok) {
      size_t offset = big->size - (size_t)keep;
      ByteArray_AppendData(big, little + keep, (size_t)(n - keep));
      return offset;
    }
  }
  return 0;
}

static void AwriteU8(uint8 *arr, int area, const uint8 *is_small, int key, uint8 value) {
  arr[key] = value;
  if (area < 128 && !is_small[area])
    arr[key + 1] = arr[key + 8] = arr[key + 9] = value;
}

static void AwriteU16(uint16 *arr, int area, const uint8 *is_small, int key, uint16 value) {
  arr[key] = value;
  if (area < 128 && !is_small[area])
    arr[key + 1] = arr[key + 8] = arr[key + 9] = value;
}

static int IsAreaHead(int i) {
  return i >= 128 || RomByte(0x82A5EC + (i & 63)) == (uint8)(i & 63);
}

// ---------------------------------------------------------------------------
// Builders
// ---------------------------------------------------------------------------

static void CopySoundBank(uint32 ea, ByteArray *out) {
  for (;;) {
    uint16 n = RomWord(ea);
    AppendU16(out, n);
    if (n == 0)
      return;
    AppendU16(out, RomWord(ea + 2));
    ea += 4;
    for (uint16 i = 0; i < n; i++) {
      ByteArray_AppendByte(out, RomByte(ea));
      ea++;
      if ((ea & 0xffff) < 0x8000)
        ea += 0x8000;
    }
  }
}

static uint32 CopyLayer(uint32 p, ByteArray *data, bool force_doors, size_t *door_abs) {
  for (;;) {
    uint16 A = RomWord(p);
    if (A == 0xffff) {
      if (force_doors) {
        ByteArray_AppendByte(data, 0xf0);
        ByteArray_AppendByte(data, 0xff);
        if (door_abs)
          *door_abs = data->size;
      }
      ByteArray_AppendByte(data, 0xff);
      ByteArray_AppendByte(data, 0xff);
      return p + 2;
    }
    if (A == 0xfff0) {
      ByteArray_AppendByte(data, 0xf0);
      ByteArray_AppendByte(data, 0xff);
      if (door_abs)
        *door_abs = data->size;
      p += 2;
      for (;;) {
        A = RomWord(p);
        if (A == 0xffff) {
          ByteArray_AppendByte(data, 0xff);
          ByteArray_AppendByte(data, 0xff);
          return p + 2;
        }
        ByteArray_AppendByte(data, RomByte(p));
        ByteArray_AppendByte(data, RomByte(p + 1));
        p += 2;
      }
    }
    ByteArray_AppendByte(data, RomByte(p));
    ByteArray_AppendByte(data, RomByte(p + 1));
    ByteArray_AppendByte(data, RomByte(p + 2));
    p += 3;
  }
}

static void CopyObjList(uint32 base, int count, ByteArray *data, ByteArray *offs) {
  for (int i = 0; i < count; i++) {
    uint32 p = base + (uint32)i * 3;
    uint32 addr = Rom24(p);
    AppendU16(offs, (uint16)data->size);
    CopyLayer(addr, data, false, NULL);
  }
}

static void AddU16Array(const char *name, const uint16 *v, int n) {
  ByteArray a = { 0 };
  ByteArray_AppendData(&a, (const uint8 *)v, (size_t)n * 2);
  AddAsset(name, &a);
}

static void AddU8Array(const char *name, const uint8 *v, int n) {
  ByteArray a = { 0 };
  ByteArray_AppendData(&a, v, (size_t)n);
  AddAsset(name, &a);
}

static void AddI8Array(const char *name, const int8 *v, int n) {
  AddU8Array(name, (const uint8 *)v, n);
}

static void AddI16Array(const char *name, const int16 *v, int n) {
  ByteArray a = { 0 };
  ByteArray_AppendData(&a, (const uint8 *)v, (size_t)n * 2);
  AddAsset(name, &a);
}

static void BuildEntranceSet(int set_idx, int n, const char *prefix, bool starting) {
  char name[64];
  uint16 *rooms = (uint16 *)calloc((size_t)n, 2);
  uint8 *rel = (uint8 *)calloc((size_t)n, 8);
  uint16 *scrollX = (uint16 *)calloc((size_t)n, 2);
  uint16 *scrollY = (uint16 *)calloc((size_t)n, 2);
  uint16 *playerX = (uint16 *)calloc((size_t)n, 2);
  uint16 *playerY = (uint16 *)calloc((size_t)n, 2);
  uint16 *cameraX = (uint16 *)calloc((size_t)n, 2);
  uint16 *cameraY = (uint16 *)calloc((size_t)n, 2);
  uint8 *blockset = (uint8 *)calloc((size_t)n, 1);
  int8 *floor = (int8 *)calloc((size_t)n, 1);
  int8 *palace = (int8 *)calloc((size_t)n, 1);
  uint8 *door_or = (uint8 *)calloc((size_t)n, 1);
  uint8 *startbg = (uint8 *)calloc((size_t)n, 1);
  uint8 *q1 = (uint8 *)calloc((size_t)n, 1);
  uint8 *q2 = (uint8 *)calloc((size_t)n, 1);
  uint16 *doors = (uint16 *)calloc((size_t)n, 2);
  uint8 *music = (uint8 *)calloc((size_t)n, 1);
  uint8 *entrance = starting ? (uint8 *)calloc((size_t)n, 1) : NULL;
  if (!rooms || !rel)
    Die("Out of memory");

  uint32 room_a = set_idx ? 0x82DB6E : 0x82C813;
  uint32 rel_a = set_idx ? 0x82DB7C : 0x82C91D;
  uint32 sx_a = set_idx ? 0x82DBB4 : 0x82CD45;
  uint32 sy_a = set_idx ? 0x82DBC2 : 0x82CE4F;
  uint32 px_a = set_idx ? 0x82DBDE : 0x82D063;
  uint32 py_a = set_idx ? 0x82DBD0 : 0x82CF59;
  uint32 cx_a = set_idx ? 0x82DBFA : 0x82D277;
  uint32 cy_a = set_idx ? 0x82DBEC : 0x82D16D;
  uint32 bs_a = set_idx ? 0x82DC08 : 0x82D381;
  uint32 fl_a = set_idx ? 0x82DC0F : 0x82D406;
  uint32 pa_a = set_idx ? 0x82DC16 : 0x82D48B;
  uint32 bg_a = set_idx ? 0x82DC1D : 0x82D595;
  uint32 q1_a = set_idx ? 0x82DC24 : 0x82D61a;
  uint32 q2_a = set_idx ? 0x82DC2B : 0x82D69F;
  uint32 dr_a = set_idx ? 0x82DC32 : 0x82D724;
  uint32 mu_a = set_idx ? 0x82DC4E : 0x82D82E;

  for (int i = 0; i < n; i++) {
    rooms[i] = RomWord(room_a + (uint32)i * 2);
    for (int j = 0; j < 8; j++)
      rel[i * 8 + j] = RomByte(rel_a + (uint32)i * 8 + (uint32)j);
    scrollX[i] = RomWord(sx_a + (uint32)i * 2);
    scrollY[i] = RomWord(sy_a + (uint32)i * 2);
    playerX[i] = RomWord(px_a + (uint32)i * 2);
    playerY[i] = RomWord(py_a + (uint32)i * 2);
    cameraX[i] = RomWord(cx_a + (uint32)i * 2);
    cameraY[i] = RomWord(cy_a + (uint32)i * 2);
    blockset[i] = RomByte(bs_a + (uint32)i);
    floor[i] = RomInt8(fl_a + (uint32)i);
    palace[i] = RomInt8(pa_a + (uint32)i);
    door_or[i] = set_idx ? 0 : (uint8)RomInt8(0x82D510 + (uint32)i);
    startbg[i] = RomByte(bg_a + (uint32)i);
    q1[i] = RomByte(q1_a + (uint32)i);
    q2[i] = RomByte(q2_a + (uint32)i);
    doors[i] = RomWord(dr_a + (uint32)i * 2);
    music[i] = RomByte(mu_a + (uint32)i);
    if (entrance)
      entrance[i] = (uint8)RomWord(0x82DC40 + (uint32)i * 2);
  }

#define ADD_PREF(suf, fn, ptr, cnt) \
  do { snprintf(name, sizeof(name), "%s%s", prefix, suf); fn(name, ptr, cnt); } while (0)
  ADD_PREF("rooms", AddU16Array, rooms, n);
  ADD_PREF("relativeCoords", AddU8Array, rel, n * 8);
  ADD_PREF("scrollX", AddU16Array, scrollX, n);
  ADD_PREF("scrollY", AddU16Array, scrollY, n);
  ADD_PREF("playerX", AddU16Array, playerX, n);
  ADD_PREF("playerY", AddU16Array, playerY, n);
  ADD_PREF("cameraX", AddU16Array, cameraX, n);
  ADD_PREF("cameraY", AddU16Array, cameraY, n);
  ADD_PREF("blockset", AddU8Array, blockset, n);
  ADD_PREF("floor", AddI8Array, floor, n);
  ADD_PREF("palace", AddI8Array, palace, n);
  ADD_PREF("doorwayOrientation", AddU8Array, door_or, n);
  ADD_PREF("startingBg", AddU8Array, startbg, n);
  ADD_PREF("quadrant1", AddU8Array, q1, n);
  ADD_PREF("quadrant2", AddU8Array, q2, n);
  ADD_PREF("doorSettings", AddU16Array, doors, n);
  if (starting)
    ADD_PREF("entrance", AddU8Array, entrance, n);
  ADD_PREF("musicTrack", AddU8Array, music, n);
#undef ADD_PREF

  free(rooms); free(rel); free(scrollX); free(scrollY);
  free(playerX); free(playerY); free(cameraX); free(cameraY);
  free(blockset); free(floor); free(palace); free(door_or);
  free(startbg); free(q1); free(q2); free(doors); free(music); free(entrance);
}

static void BuildDungeonRooms(void) {
  ByteArray data = { 0 };
  uint16 offsets[320];
  uint16 door_offsets[320];
  ByteArray headers = { 0 };
  uint16 header_offs[320];
  ByteArray chests = { 0 };
  uint16 sign_texts[320];
  uint16 pits[64];
  int npits = 0;

  typedef struct { uint8 data; uint8 big; } ChestEnt;
  ChestEnt *cmap[320];
  int cn[320];
  memset(cmap, 0, sizeof(cmap));
  memset(cn, 0, sizeof(cn));
  for (int i = 0; i < 168; i++) {
    uint32 ea = 0x81e96e + (uint32)i * 3;
    uint16 room = RomWord(ea);
    int idx = room & 0x7fff;
    if (idx < 320) {
      cmap[idx] = (ChestEnt *)realloc(cmap[idx], (size_t)(cn[idx] + 1) * sizeof(ChestEnt));
      cmap[idx][cn[idx]].data = RomByte(ea + 2);
      cmap[idx][cn[idx]].big = (room & 0x8000) != 0;
      cn[idx]++;
    }
  }

  uint8 pitset[320];
  memset(pitset, 0, sizeof(pitset));
  for (int i = 0; i < 57; i++) {
    uint16 r = RomWord(0x80990C + (uint32)i * 2);
    if (r < 320)
      pitset[r] = 1;
  }

  for (int i = 0; i < 320; i++) {
    uint32 rp = 0x1f8000 + (uint32)i * 3;
    uint32 room_addr = Rom24(rp);
    uint32 hp = 0x40000 | RomWord(0x4f502 + (uint32)i * 2);
    if (hp == 0x4FFEF)
      hp = 0x82EDC5;
    if (pitset[i])
      pits[npits++] = (uint16)i;
    offsets[i] = (uint16)data.size;
    ByteArray_AppendByte(&data, RomByte(room_addr));
    ByteArray_AppendByte(&data, RomByte(room_addr + 1));
    uint32 p = room_addr + 2;
    p = CopyLayer(p, &data, false, NULL);
    p = CopyLayer(p, &data, false, NULL);
    size_t door_abs = 0;
    CopyLayer(p, &data, true, &door_abs);
    door_offsets[i] = (uint16)door_abs;

    uint8 hdr[14];
    for (int j = 0; j < 14; j++)
      hdr[j] = RomByte(hp + (uint32)j);
    hdr[0] = (uint8)((hdr[0] & 0xe0) | (hdr[0] & 0x1c) | (hdr[0] & 1));
    hdr[8] &= 3;
    header_offs[i] = (uint16)AppendScanBytes(&headers, hdr, 14);
    sign_texts[i] = RomWord(0x87F61D + (uint32)i * 2);
    for (int c = 0; c < cn[i]; c++) {
      ByteArray_AppendByte(&chests, (uint8)i);
      ByteArray_AppendByte(&chests, (uint8)((i >> 8) | (cmap[i][c].big ? 0x80 : 0)));
      ByteArray_AppendByte(&chests, cmap[i][c].data);
    }
  }
  for (int i = 0; i < 320; i++)
    free(cmap[i]);

  AddAsset("kDungeonRoom", &data);
  AddU16Array("kDungeonRoomOffs", offsets, 320);
  AddU16Array("kDungeonRoomDoorOffs", door_offsets, 320);
  AddAsset("kDungeonRoomHeaders", &headers);
  AddU16Array("kDungeonRoomHeadersOffs", header_offs, 320);
  AddAsset("kDungeonRoomChests", &chests);
  AddU16Array("kDungeonRoomTeleMsg", sign_texts, 320);
  AddU16Array("kDungeonPitsHurtPlayer", pits, npits);

  BuildEntranceSet(0, 133, "kEntranceData_", false);
  BuildEntranceSet(1, 7, "kStartingPoint_", true);

  ByteArray def = { 0 }, defoff = { 0 };
  CopyObjList(0x84EF2F, 8, &def, &defoff);
  AddAsset("kDungeonRoomDefault", &def);
  AddAsset("kDungeonRoomDefaultOffs", &defoff);
  ByteArray ov = { 0 }, ovoff = { 0 };
  CopyObjList(0x84ECC0, 19, &ov, &ovoff);
  AddAsset("kDungeonRoomOverlay", &ov);
  AddAsset("kDungeonRoomOverlayOffs", &ovoff);

  ByteArray secrets = { 0 };
  for (int i = 0; i < 640; i++)
    ByteArray_AppendByte(&secrets, 0);
  for (int i = 0; i < 320; i++) {
    secrets.data[i * 2] = 0xff;
    secrets.data[i * 2 + 1] = 0xff;
  }
  for (int i = 0; i < 320; i++) {
    uint32 ea = 0x810000 | RomWord(0x81db69 + (uint32)i * 2);
    int has = RomWord(ea) != 0xffff;
    if (!has)
      continue;
    uint16 at = (uint16)secrets.size;
    secrets.data[i * 2] = (uint8)at;
    secrets.data[i * 2 + 1] = (uint8)(at >> 8);
    while (RomWord(ea) != 0xffff) {
      RomAppend(&secrets, ea, 3);
      ea += 3;
    }
    ByteArray_AppendByte(&secrets, 0xff);
    ByteArray_AppendByte(&secrets, 0xff);
  }
  uint16 empty_at = (uint16)(secrets.size - 2);
  for (int i = 0; i < 320; i++) {
    if (secrets.data[i * 2] == 0xff && secrets.data[i * 2 + 1] == 0xff) {
      secrets.data[i * 2] = (uint8)empty_at;
      secrets.data[i * 2 + 1] = (uint8)(empty_at >> 8);
    }
  }
  AddAsset("kDungeonSecrets", &secrets);
  AddRomWords("kDungAttrsForTile_Offs", 0x8e9000, 21);
  AddRomBytes("kDungAttrsForTile", 0x8e902a, 1024);
  AddRomWords("kMovableBlockDataInit", 0x84f1de, 198);
  AddRomWords("kTorchDataInit", 0x84F36A, 144);
  AddRomWords("kTorchDataJunk", 0x84F48a, 48);
}

static void BuildDungeonSprites(void) {
  ByteArray data = { 0 };
  uint16 offsets[320];
  memset(offsets, 0, sizeof(offsets));
  ByteArray_AppendByte(&data, 0);
  ByteArray_AppendByte(&data, 0xff);
  for (int i = 0; i < 320; i++) {
    uint32 ea = 0x890000 + RomWord(0x89D62E + (uint32)i * 2);
    uint8 sortmode = RomByte(ea);
    ea++;
    int n = 0;
    uint32 scan = ea;
    while (RomByte(scan) != 0xff) {
      n += 3;
      scan += 3;
    }
    if (n == 0 && sortmode == 0)
      continue;
    offsets[i] = (uint16)data.size;
    ByteArray_AppendByte(&data, sortmode);
    RomAppend(&data, ea, n);
    ByteArray_AppendByte(&data, 0xff);
  }
  AddAsset("kDungeonSprites", &data);
  AddU16Array("kDungeonSpriteOffs", offsets, 320);
}

static void BuildMap32(void) {
  uint16 tab[8872][4];
  for (int i = 0; i < 2218; i++) {
    uint16 t[4][4];
    const uint32 bases[4] = { 0x838000, 0x83b400, 0x848000, 0x84b400 };
    for (int tix = 0; tix < 4; tix++) {
      uint32 ea = bases[tix] + (uint32)i * 6;
      uint8 ov[6];
      for (int j = 0; j < 6; j++)
        ov[j] = RomByte(ea + (uint32)j);
      t[tix][0] = (uint16)(ov[0] | ((ov[4] >> 4) << 8));
      t[tix][1] = (uint16)(ov[1] | ((ov[4] & 0xf) << 8));
      t[tix][2] = (uint16)(ov[2] | ((ov[5] >> 4) << 8));
      t[tix][3] = (uint16)(ov[3] | ((ov[5] & 0xf) << 8));
    }
    for (int j = 0; j < 4; j++) {
      tab[i * 4 + j][0] = t[0][j];
      tab[i * 4 + j][1] = t[1][j];
      tab[i * 4 + j][2] = t[2][j];
      tab[i * 4 + j][3] = t[3][j];
    }
  }
  ByteArray res[4] = { { 0 }, { 0 }, { 0 }, { 0 } };
  for (int i = 0; i < 8872; i += 4) {
    for (int j = 0; j < 4; j++) {
      uint16 a0 = tab[i][j], a1 = tab[i + 1][j], a2 = tab[i + 2][j], a3 = tab[i + 3][j];
      ByteArray_AppendByte(&res[j], (uint8)a0);
      ByteArray_AppendByte(&res[j], (uint8)a1);
      ByteArray_AppendByte(&res[j], (uint8)a2);
      ByteArray_AppendByte(&res[j], (uint8)a3);
      ByteArray_AppendByte(&res[j], (uint8)(((a0 >> 8) << 4) | (a1 >> 8)));
      ByteArray_AppendByte(&res[j], (uint8)(((a2 >> 8) << 4) | (a3 >> 8)));
    }
  }
  AddAsset("kMap32ToMap16_0", &res[0]);
  AddAsset("kMap32ToMap16_1", &res[1]);
  AddAsset("kMap32ToMap16_2", &res[2]);
  AddAsset("kMap32ToMap16_3", &res[3]);
}

static void BuildImages(void) {
  ByteArray spr[108];
  memset(spr, 0, sizeof(spr));
  for (int i = 0; i < 108; i++) {
    if (i < 12)
      RomAppend(&spr[i], kCompSpritePtrs[i], 0x600);
    else
      RomAppend(&spr[i], kCompSpritePtrs[i], RomDecompLen(kCompSpritePtrs[i], false));
  }
  ByteArray packed = { 0 };
  PackArrays(&packed, spr, 108);
  for (int i = 0; i < 108; i++)
    BA_Clear(&spr[i]);
  AddAsset("kSprGfx", &packed);

  ByteArray bg[115];
  memset(bg, 0, sizeof(bg));
  for (int i = 0; i < 115; i++)
    RomAppend(&bg[i], kCompBgPtrs[i], RomDecompLen(kCompBgPtrs[i], false));
  packed = (ByteArray){ 0 };
  PackArrays(&packed, bg, 115);
  for (int i = 0; i < 115; i++)
    BA_Clear(&bg[i]);
  AddAsset("kBgGfx", &packed);
}

static int AlphabetIndex(const char *s, size_t n) {
  for (int i = 0; i < (int)countof(kTextAlphabetUS); i++) {
    if (strlen(kTextAlphabetUS[i]) == n && memcmp(kTextAlphabetUS[i], s, n) == 0)
      return i;
  }
  return -1;
}

static void EncodeDictionaryWord(const char *s, ByteArray *out) {
  while (*s) {
    int idx = AlphabetIndex(s, 1);
    if (idx < 0)
      Die("Dictionary character not in US alphabet");
    ByteArray_AppendByte(out, (uint8)idx);
    s++;
  }
}

static void DecodeUsDialogue(ByteArray *strs, int *n_out) {
  uint32 addrs[2] = { 0x9c8000, 0x8edf40 };
  uint32 p = addrs[0];
  int rom_idx = 1;
  int n = 0;
  ByteArray cur = { 0 };
  for (;;) {
    uint8 c = RomByte(p);
    ByteArray_AppendByte(&cur, c);
    int l = 1;
    if (c >= 0x67 && c < 0x80)
      l = kTextCmdLenUS[c - 0x67];
    p += (uint32)l;
    if (c == 0x7f) {
      if (cur.size > 0 && cur.data[cur.size - 1] == 0x7f)
        cur.size--;
      strs[n++] = cur;
      cur = (ByteArray){ 0 };
      continue;
    }
    if (c < 0x67) {
      continue;
    }
    if (c < 0x80) {
      if (l == 2)
        ByteArray_AppendByte(&cur, RomByte(p - 1));
      continue;
    }
    if (c == 0xff) {
      BA_Clear(&cur);
      break;
    }
    if (c == 0x80) {
      BA_Clear(&cur);
      p = addrs[rom_idx++];
      continue;
    }
  }
  if (n == 396) {
    for (int i = n; i > 4; i--)
      strs[i] = strs[i - 1];
    strs[4] = (ByteArray){ 0 };
    ByteArray_AppendData(&strs[4], kExtraDialogue, sizeof(kExtraDialogue));
    n++;
  }
  *n_out = n;
}

static void BuildDialogue(void) {
  ByteArray dicts[countof(kTextDictionaryUS)];
  memset(dicts, 0, sizeof(dicts));
  for (int i = 0; i < (int)countof(kTextDictionaryUS); i++)
    EncodeDictionaryWord(kTextDictionaryUS[i], &dicts[i]);
  ByteArray dict_packed = { 0 };
  PackArrays(&dict_packed, dicts, (int)countof(kTextDictionaryUS));
  for (int i = 0; i < (int)countof(kTextDictionaryUS); i++)
    BA_Clear(&dicts[i]);

  ByteArray strs[400];
  memset(strs, 0, sizeof(strs));
  int nstr = 0;
  DecodeUsDialogue(strs, &nstr);
  ByteArray dialogue_packed = { 0 };
  PackArrays(&dialogue_packed, strs, nstr);
  for (int i = 0; i < nstr; i++)
    BA_Clear(&strs[i]);

  ByteArray pair[2] = { dict_packed, dialogue_packed };
  ByteArray lang = { 0 };
  PackArrays(&lang, pair, 2);
  BA_Clear(&dict_packed);
  BA_Clear(&dialogue_packed);
  ByteArray langs[1] = { lang };
  ByteArray kdlg = { 0 };
  PackArrays(&kdlg, langs, 1);
  BA_Clear(&lang);
  AddAsset("kDialogue", &kdlg);

  ByteArray font_data = { 0 }, font_w = { 0 };
  RomAppend(&font_data, 0x8e8000, 256 * 16);
  RomAppend(&font_w, 0x8ECADF, 99);
  ByteArray font_parts[2] = { font_data, font_w };
  ByteArray fonts = { 0 };
  PackArrays(&fonts, font_parts, 2);
  BA_Clear(&font_data);
  BA_Clear(&font_w);
  ByteArray fontwrap[1] = { fonts };
  ByteArray kfont = { 0 };
  PackArrays(&kfont, fontwrap, 1);
  BA_Clear(&fonts);
  AddAsset("kDialogueFont", &kfont);

  ByteArray langname = { 0 }, flags = { 0 };
  ByteArray_AppendData(&langname, (const uint8 *)"us", 2);
  ByteArray_AppendByte(&flags, 0);
  ByteArray_AppendByte(&flags, 0);
  ByteArray_AppendByte(&flags, 0);
  ByteArray map_parts[2] = { langname, flags };
  ByteArray map1 = { 0 };
  PackArrays(&map1, map_parts, 2);
  BA_Clear(&langname);
  BA_Clear(&flags);
  ByteArray maps[1] = { map1 };
  ByteArray kmap = { 0 };
  PackArrays(&kmap, maps, 1);
  BA_Clear(&map1);
  AddAsset("kDialogueMap", &kmap);
}

static void BuildDungeonMap(void) {
  static const int kSizes[14] = { 75, 125, 50, 75, 175, 75, 50, 75, 50, 200, 150, 75, 100, 200 };
  ByteArray r[14], r2[14];
  memset(r, 0, sizeof(r));
  memset(r2, 0, sizeof(r2));
  for (int i = 0; i < 14; i++) {
    uint32 addr = 0xa0000 + RomWord(0x8AF605 + (uint32)i * 2);
    RomAppend(&r[i], addr, kSizes[i]);
    int nonzero = kSizes[i];
    for (size_t j = 0; j < r[i].size; j++) {
      if (r[i].data[j] == 0xf)
        nonzero--;
    }
    addr = 0xa0000 + RomWord(0x8AFBE4 + (uint32)i * 2);
    RomAppend(&r2[i], addr, nonzero);
  }
  ByteArray a = { 0 }, b = { 0 };
  PackArrays(&a, r, 14);
  PackArrays(&b, r2, 14);
  for (int i = 0; i < 14; i++) {
    BA_Clear(&r[i]);
    BA_Clear(&r2[i]);
  }
  AddAsset("kDungMap_FloorLayout", &a);
  AddAsset("kDungMap_Tiles", &b);
}

static int DecodeTilemapLen(uint32 p) {
  uint32 p_org = p;
  while (!(RomByte(p) & 0x80)) {
    int is_memset = RomByte(p + 2) & 0x40;
    int len = (((RomByte(p + 2) * 256 + RomByte(p + 3)) & 0x3fff) + 1);
    p += 4;
    p += is_memset ? 2 : len;
  }
  return (int)(p - p_org + 1);
}

static void BuildOverworldTables(void) {
  uint8 is_small[192];
  for (int i = 0; i < 192; i++)
    is_small[i] = RomByte(0x82F88D + (uint32)i);
  AddU8Array("kOverworldMapIsSmall", is_small, 192);

  uint8 aux[128];
  uint8 pal[136];
  uint16 signs[128];
  uint8 music[256];
  uint8 music2[96];
  memset(aux, 0, sizeof(aux));
  memset(pal, 0, sizeof(pal));
  memset(signs, 0, sizeof(signs));
  memset(music, 0, sizeof(music));
  memset(music2, 0, sizeof(music2));
  for (int i = 0; i < 160; i++) {
    if (!IsAreaHead(i))
      continue;
    if (i < 128)
      AwriteU8(aux, i, is_small, i, RomByte(0x80FC9C + (uint32)i));
    if (i < 136)
      AwriteU8(pal, i, is_small, i, RomByte(0x80FD1C + (uint32)i));
    if (i < 128)
      AwriteU16(signs, i, is_small, i, RomWord(0x87F51D + (uint32)i * 2));
    if (i < 64) {
      AwriteU8(music, i, is_small, i, RomByte(0x82C303 + (uint32)i));
      AwriteU8(music, i, is_small, i + 64, RomByte(0x82C303 + (uint32)i + 64));
      AwriteU8(music, i, is_small, i + 128, RomByte(0x82C303 + (uint32)i + 128));
      AwriteU8(music, i, is_small, i + 192, RomByte(0x82C303 + (uint32)i + 192));
    } else if (i < 160) {
      AwriteU8(music2, i, is_small, i - 64, RomByte(0x82C403 + (uint32)i - 64));
    }
  }
  AddU8Array("kOverworldAuxTileThemeIndexes", aux, 128);
  AddU8Array("kOverworldBgPalettes", pal, 136);
  AddU16Array("kOverworld_SignText", signs, 128);
  AddU8Array("kOwMusicSets", music, 256);
  AddU8Array("kOwMusicSets2", music2, 96);

  uint16 bird_screen[17] = { 0 }, bird_load[17] = { 0 }, bird_sx[17] = { 0 }, bird_sy[17] = { 0 };
  uint16 bird_px[17] = { 0 }, bird_py[17] = { 0 }, bird_cx[17] = { 0 }, bird_cy[17] = { 0 };
  int8 bird_u1[17] = { 0 }, bird_u3[17] = { 0 };
  uint16 whirl[8] = { 0 };
  int next_whirl = 0;

  typedef struct {
    uint16 screen, load, sy, sx, py, px, cy, cx;
    int8 u1, u3;
    int bird;
    uint16 whirl;
  } TravelRec;
  TravelRec recs[17];
  for (int ti = 0; ti < 17; ti++) {
    recs[ti].screen = RomWord(0x82EAE5 + (uint32)ti * 2);
    recs[ti].load = RomWord(0x82EB07 + (uint32)ti * 2);
    recs[ti].sy = RomWord(0x82EB29 + (uint32)ti * 2);
    recs[ti].sx = RomWord(0x82EB4B + (uint32)ti * 2);
    recs[ti].py = RomWord(0x82EB6D + (uint32)ti * 2);
    recs[ti].px = RomWord(0x82EB8F + (uint32)ti * 2);
    recs[ti].cy = RomWord(0x82EBB1 + (uint32)ti * 2);
    recs[ti].cx = RomWord(0x82EBD3 + (uint32)ti * 2);
    recs[ti].u1 = RomInt8(0x82EBF5 + (uint32)ti * 2);
    recs[ti].u3 = RomInt8(0x82EC17 + (uint32)ti * 2);
    recs[ti].bird = (ti < 9) ? ti : -1;
    recs[ti].whirl = (ti >= 9) ? RomWord(0x82ECF8 + (uint32)(ti - 9) * 2) : 0;
  }
  for (int i = 0; i < 160; i++) {
    if (!IsAreaHead(i))
      continue;
    for (int ti = 0; ti < 17; ti++) {
      if (recs[ti].screen != (uint16)i)
        continue;
      int j;
      if (recs[ti].bird >= 0) {
        j = recs[ti].bird;
      } else {
        whirl[next_whirl] = recs[ti].whirl;
        j = next_whirl + 9;
        next_whirl++;
      }
      bird_screen[j] = recs[ti].screen;
      bird_load[j] = recs[ti].load;
      bird_sx[j] = recs[ti].sx;
      bird_sy[j] = recs[ti].sy;
      bird_px[j] = recs[ti].px;
      bird_py[j] = recs[ti].py;
      bird_cx[j] = recs[ti].cx;
      bird_cy[j] = recs[ti].cy;
      bird_u1[j] = recs[ti].u1;
      bird_u3[j] = recs[ti].u3;
    }
  }
  AddU16Array("kBirdTravel_ScreenIndex", bird_screen, 17);
  AddU16Array("kBirdTravel_Map16LoadSrcOff", bird_load, 17);
  AddU16Array("kBirdTravel_ScrollX", bird_sx, 17);
  AddU16Array("kBirdTravel_ScrollY", bird_sy, 17);
  AddU16Array("kBirdTravel_LinkXCoord", bird_px, 17);
  AddU16Array("kBirdTravel_LinkYCoord", bird_py, 17);
  AddU16Array("kBirdTravel_CameraXScroll", bird_cx, 17);
  AddU16Array("kBirdTravel_CameraYScroll", bird_cy, 17);
  AddI8Array("kBirdTravel_Unk1", bird_u1, 17);
  AddI8Array("kBirdTravel_Unk3", bird_u3, 17);
  AddU16Array("kWhirlpoolAreas", whirl, 8);

  AddRomWords("kOverworld_Entrance_Area", 0x9BB96F, 129);
  AddRomWords("kOverworld_Entrance_Pos", 0x9BBA71, 129);
  AddRomBytes("kOverworld_Entrance_Id", 0x9BBB73, 129);

  typedef struct { uint16 entrance, pos, area; } Hole;
  Hole holes[19];
  for (int i = 0; i < 19; i++) {
    holes[i].pos = RomWord(0x9BB800 + (uint32)i * 2);
    holes[i].area = RomWord(0x9BB826 + (uint32)i * 2);
    holes[i].entrance = RomByte(0x9BB84C + (uint32)i);
  }
  for (int a = 0; a < 18; a++) {
    for (int b = a + 1; b < 19; b++) {
      if (holes[b].entrance < holes[a].entrance ||
          (holes[b].entrance == holes[a].entrance && holes[b].pos < holes[a].pos) ||
          (holes[b].entrance == holes[a].entrance && holes[b].pos == holes[a].pos &&
           holes[b].area < holes[a].area)) {
        Hole t = holes[a];
        holes[a] = holes[b];
        holes[b] = t;
      }
    }
  }
  uint16 harea[19], hpos[19];
  uint8 hent[19];
  for (int i = 0; i < 19; i++) {
    harea[i] = holes[i].area;
    hpos[i] = holes[i].pos;
    hent[i] = (uint8)holes[i].entrance;
  }
  AddU16Array("kFallHole_Area", harea, 19);
  AddU16Array("kFallHole_Pos", hpos, 19);
  AddU8Array("kFallHole_Entrances", hent, 19);

  uint8 exit_screen[79];
  uint16 exit_room[79], exit_load[79], exit_sx[79], exit_sy[79];
  uint16 exit_x[79], exit_y[79], exit_cx[79], exit_cy[79];
  uint16 exit_nd[79], exit_fd[79];
  int8 exit_u1[79], exit_u3[79];
  for (int i = 0; i < 79; i++) {
    exit_screen[i] = RomByte(0x82DE28 + (uint32)i);
    exit_room[i] = RomWord(0x82dd8a + (uint32)i * 2);
    exit_load[i] = RomWord(0x82DE77 + (uint32)i * 2);
    exit_sx[i] = RomWord(0x82DFB3 + (uint32)i * 2);
    exit_sy[i] = RomWord(0x82DF15 + (uint32)i * 2);
    exit_x[i] = RomWord(0x82E0EF + (uint32)i * 2);
    exit_y[i] = RomWord(0x82E051 + (uint32)i * 2);
    exit_cx[i] = RomWord(0x82E22B + (uint32)i * 2);
    exit_cy[i] = RomWord(0x82E18D + (uint32)i * 2);
    exit_nd[i] = RomWord(0x82E367 + (uint32)i * 2);
    exit_fd[i] = RomWord(0x82E405 + (uint32)i * 2);
    exit_u1[i] = RomInt8(0x82E2C9 + (uint32)i);
    exit_u3[i] = RomInt8(0x82E318 + (uint32)i);
  }
  AddU8Array("kExitData_ScreenIndex", exit_screen, 79);
  AddU16Array("kExitDataRooms", exit_room, 79);
  AddU16Array("kExitData_Map16LoadSrcOff", exit_load, 79);
  AddU16Array("kExitData_ScrollX", exit_sx, 79);
  AddU16Array("kExitData_ScrollY", exit_sy, 79);
  AddU16Array("kExitData_XCoord", exit_x, 79);
  AddU16Array("kExitData_YCoord", exit_y, 79);
  AddU16Array("kExitData_CameraXScroll", exit_cx, 79);
  AddU16Array("kExitData_CameraYScroll", exit_cy, 79);
  AddU16Array("kExitData_NormalDoor", exit_nd, 79);
  AddU16Array("kExitData_FancyDoor", exit_fd, 79);
  AddI8Array("kExitData_Unk1", exit_u1, 79);
  AddI8Array("kExitData_Unk3", exit_u3, 79);

  uint16 sp_top[16] = { 0 }, sp_bot[16] = { 0 }, sp_left[16] = { 0 }, sp_right[16] = { 0 };
  int16 sp_t4[16] = { 0 }, sp_t5[16] = { 0 }, sp_t6[16] = { 0 }, sp_t7[16] = { 0 };
  uint16 sp_edge[16] = { 0 };
  uint8 sp_dir[16] = { 0 }, sp_sg[16] = { 0 }, sp_ag[16] = { 0 }, sp_pb[16] = { 0 }, sp_ps[16] = { 0 };
  for (int ei = 0; ei < 79; ei++) {
    uint16 room = exit_room[ei];
    if (room < 0x180 || room >= 0x190)
      continue;
    int j = room - 0x180;
    sp_dir[j] = RomByte(0x82E801 + (uint32)j);
    sp_sg[j] = RomByte(0x82E811 + (uint32)j);
    sp_ag[j] = RomByte(0x82E821 + (uint32)j);
    sp_pb[j] = RomByte(0x82E831 + (uint32)j);
    sp_ps[j] = RomByte(0x82E841 + (uint32)j);
    sp_top[j] = RomWord(0x82e6e1 + (uint32)j * 2);
    sp_bot[j] = RomWord(0x82e701 + (uint32)j * 2);
    sp_left[j] = RomWord(0x82e721 + (uint32)j * 2);
    sp_right[j] = RomWord(0x82e741 + (uint32)j * 2);
    sp_edge[j] = RomWord(0x82E7E1 + (uint32)j * 2);
    sp_t4[j] = RomInt16(0x82e761 + (uint32)j * 2);
    sp_t6[j] = RomInt16(0x82e781 + (uint32)j * 2);
    sp_t5[j] = RomInt16(0x82e7a1 + (uint32)j * 2);
    sp_t7[j] = RomInt16(0x82e7c1 + (uint32)j * 2);
  }
  AddU16Array("kSpExit_Top", sp_top, 16);
  AddU16Array("kSpExit_Bottom", sp_bot, 16);
  AddU16Array("kSpExit_Left", sp_left, 16);
  AddU16Array("kSpExit_Right", sp_right, 16);
  AddI16Array("kSpExit_Tab4", sp_t4, 16);
  AddI16Array("kSpExit_Tab5", sp_t5, 16);
  AddI16Array("kSpExit_Tab6", sp_t6, 16);
  AddI16Array("kSpExit_Tab7", sp_t7, 16);
  AddU16Array("kSpExit_LeftEdgeOfMap", sp_edge, 16);
  AddU8Array("kSpExit_Dir", sp_dir, 16);
  AddU8Array("kSpExit_SprGfx", sp_sg, 16);
  AddU8Array("kSpExit_AuxGfx", sp_ag, 16);
  AddU8Array("kSpExit_PalBg", sp_pb, 16);
  AddU8Array("kSpExit_PalSpr", sp_ps, 16);

  uint16 sec_offs[128];
  for (int i = 0; i < 128; i++)
    sec_offs[i] = 0xffff;
  ByteArray secblob = { 0 };
  for (int i = 0; i < 160; i++) {
    if (!IsAreaHead(i) || i >= 128)
      continue;
    uint32 ea = 0x9b0000 | RomWord(0x9BC2F9 + (uint32)i * 2);
    if (RomWord(ea) == 0xffff)
      continue;
    AwriteU16(sec_offs, i, is_small, i, (uint16)secblob.size);
    while (RomWord(ea) != 0xffff) {
      RomAppend(&secblob, ea, 3);
      ea += 3;
    }
    ByteArray_AppendByte(&secblob, 0xff);
    ByteArray_AppendByte(&secblob, 0xff);
  }
  uint16 empty = (uint16)(secblob.size >= 2 ? secblob.size - 2 : 0);
  for (int i = 0; i < 128; i++) {
    if (sec_offs[i] == 0xffff)
      sec_offs[i] = empty;
  }
  AddU16Array("kOverworldSecrets_Offs", sec_offs, 128);
  AddAsset("kOverworldSecrets", &secblob);

  uint16 spr_offs[144 * 3];
  memset(spr_offs, 0, sizeof(spr_offs));
  ByteArray sprites = { 0 };
  ByteArray_AppendByte(&sprites, 0xff);
  uint8 gfx[256];
  uint8 spal[256];
  memset(gfx, 0, sizeof(gfx));
  memset(spal, 0, sizeof(spal));

  const uint32 spr_bases[4] = { 0x89C881, 0x89C901, 0x89CA21, 0x89CA21 };
  const int spr_start[4] = { 0, 0, 0, 64 };
  const int spr_end[4] = { 64, 64, 64, 144 };
  const int spr_info[4] = { 0, 1, 2, 3 };
  for (int pass = 0; pass < 4; pass++) {
    for (int i = 0; i < 160; i++) {
      if (!IsAreaHead(i) || i < spr_start[pass] || i >= spr_end[pass])
        continue;
      if (i < 128) {
        uint8 g = RomByte(0x80FA41 + (uint32)(i & 63) + (uint32)spr_info[pass] * 64);
        uint8 p = RomByte(0x80FB41 + (uint32)(i & 63) + (uint32)spr_info[pass] * 64);
        AwriteU8(gfx, i, is_small, (i & 63) + spr_info[pass] * 64, g);
        AwriteU8(spal, i, is_small, (i & 63) + spr_info[pass] * 64, p);
      }
      uint32 ea = 0x890000 + RomWord(spr_bases[pass] + (uint32)i * 2);
      if (RomByte(ea) == 0xff)
        continue;
      if (pass == 0) {
        spr_offs[0 * 144 + i] = (uint16)sprites.size;
      } else if (pass == 1) {
        spr_offs[1 * 144 + i] = (uint16)sprites.size;
      } else if (pass == 2) {
        spr_offs[2 * 144 + i] = (uint16)sprites.size;
      } else {
        spr_offs[1 * 144 + i] = (uint16)sprites.size;
        spr_offs[2 * 144 + i] = (uint16)sprites.size;
      }
      while (RomByte(ea) != 0xff) {
        RomAppend(&sprites, ea, 3);
        ea += 3;
      }
      ByteArray_AppendByte(&sprites, 0xff);
    }
  }
  AddU16Array("kOverworldSpriteOffs", spr_offs, 144 * 3);
  AddAsset("kOverworldSprites", &sprites);
  AddU8Array("kOverworldSpriteGfx", gfx, 256);
  AddU8Array("kOverworldSpritePalettes", spal, 256);
  AddRomBytes("kMap8DataToTileAttr", 0x8E9459, 512);
  AddRomBytes("kSomeTileAttr", 0x9bf110, 3824);
}

static void BuildAllAssets(void) {
  ByteArray a = { 0 };
  CopySoundBank(0x998000, &a);
  AddAsset("kSoundBank_intro", &a);
  a = (ByteArray){ 0 };
  CopySoundBank(0x9b8000, &a);
  AddAsset("kSoundBank_indoor", &a);
  a = (ByteArray){ 0 };
  CopySoundBank(0x9ad380, &a);
  AddAsset("kSoundBank_ending", &a);

  BuildDungeonRooms();

  a = (ByteArray){ 0 };
  RomDecomp(0x83e800, true, &a, NULL);
  AddAsset("kEnemyDamageData", &a);
  AddRomBytes("kLinkGraphics", 0x108000, 0x7000);
  BuildDungeonSprites();
  BuildMap32();
  BuildImages();

  AddRomBytes("kOverworldMapGfx", 0x18c000, 0x4000);
  AddRomBytes("kLightOverworldTilemap", 0xac727, 4096);
  AddRomBytes("kDarkOverworldTilemap", 0xaD727, 1024);
  AddRomWords("kPredefinedTileData", 0x9B52, 6438);
  AddRomWords("kMap16ToMap8", 0x8f8000, 3752 * 4);
  AddRomBytes("kGeneratedWishPondItem", 0x888450, 256);
  AddRomBytes("kGeneratedBombosArr", 0x8890FC, 256);
  AddRomBytes("kGeneratedEndSequence15", 0x8ead25, 256);
  AddRomBytes("kEnding_Credits_Text", 0x8EB178, 1989);
  AddRomWords("kEnding_Credits_Offs", 0x8EB93d, 394);
  AddRomWords("kEnding_MapData", 0x8EB038, 160);
  AddRomWords("kEnding0_Offs", 0x8EC2E1, 17);
  AddRomBytes("kEnding0_Data", 0x8EBF4C, 917);
  AddRomWords("kPalette_DungBgMain", 0x9BD734, 1800);
  AddRomWords("kPalette_MainSpr", 0x9BD218, 120);
  AddRomWords("kPalette_ArmorAndGloves", 0x9BD308, 75);
  AddRomWords("kPalette_Sword", 0x9BD630, 12);
  AddRomWords("kPalette_Shield", 0x9BD648, 12);
  AddRomWords("kPalette_SpriteAux3", 0x9BD39E, 84);
  AddRomWords("kPalette_MiscSprite_Indoors", 0x9BD446, 77);
  AddRomWords("kPalette_SpriteAux1", 0x9BD4E0, 168);
  AddRomWords("kPalette_OverworldBgMain", 0x9BE6C8, 210);
  AddRomWords("kPalette_OverworldBgAux12", 0x9BE86C, 420);
  AddRomWords("kPalette_OverworldBgAux3", 0x9BE604, 98);
  AddRomWords("kPalette_PalaceMapBg", 0x9BE544, 96);
  AddRomWords("kPalette_PalaceMapSpr", 0x9BD70A, 21);
  AddRomWords("kHudPalData", 0x9BD660, 64);
  AddRomWords("kOverworldMapPaletteData", 0x8ADB27, 256);

  BuildDialogue();
  BuildDungeonMap();

  static const uint32 kTileSrcs[6] = { 0xcdd6d, 0xce7bf, 0xce2a8, 0xce63c, 0xce456, 0xeda9c };
  static const char *const kTileNames[6] = {
    "kBgTilemap_0", "kBgTilemap_1", "kBgTilemap_2", "kBgTilemap_3", "kBgTilemap_4", "kBgTilemap_5",
  };
  for (int i = 0; i < 6; i++)
    AddRomBytes(kTileNames[i], kTileSrcs[i], DecodeTilemapLen(kTileSrcs[i]));

  ByteArray hib[160], lob[160];
  memset(hib, 0, sizeof(hib));
  memset(lob, 0, sizeof(lob));
  for (int i = 0; i < 160; i++) {
    uint32 addr = Rom24(0x82F94D + (uint32)i * 3);
    RomAppend(&hib[i], addr, RomDecompLen(addr, true));
    addr = Rom24(0x82FB2D + (uint32)i * 3);
    RomAppend(&lob[i], addr, RomDecompLen(addr, true));
  }
  ByteArray ph = { 0 }, pl = { 0 };
  PackArrays(&ph, hib, 160);
  PackArrays(&pl, lob, 160);
  for (int i = 0; i < 160; i++) {
    BA_Clear(&hib[i]);
    BA_Clear(&lob[i]);
  }
  AddAsset("kOverworld_Hibytes_Comp", &ph);
  AddAsset("kOverworld_Lobytes_Comp", &pl);
  BuildOverworldTables();
}

static void WriteAssetsFile(const char *path) {
  if (g_nassets != kNumberOfAssets)
    Die("Extract produced the wrong number of assets");

  ByteArray key_sig = { 0 };
  for (int i = 0; i < g_nassets; i++) {
    ByteArray_AppendData(&key_sig, (const uint8 *)g_assets[i].name, strlen(g_assets[i].name));
    ByteArray_AppendByte(&key_sig, 0);
  }

  ByteArray file = { 0 };
  ByteArray_AppendData(&file, kAssetsSigBytes, sizeof(kAssetsSigBytes));
  for (int i = 0; i < 32; i++)
    ByteArray_AppendByte(&file, 0);
  AppendU32(&file, (uint32)g_nassets);
  AppendU32(&file, (uint32)key_sig.size);
  for (int i = 0; i < g_nassets; i++)
    AppendU32(&file, (uint32)g_assets[i].data.size);
  ByteArray_AppendData(&file, key_sig.data, key_sig.size);
  BA_Clear(&key_sig);

  for (int i = 0; i < g_nassets; i++) {
    while (file.size & 3)
      ByteArray_AppendByte(&file, 0);
    ByteArray_AppendData(&file, g_assets[i].data.data, g_assets[i].data.size);
  }

  FILE *f = fopen(path, "wb");
  if (!f)
    Die("Failed to write zelda3_assets.dat");
  if (fwrite(file.data, 1, file.size, f) != file.size)
    Die("Failed to write zelda3_assets.dat");
  fclose(f);
  BA_Clear(&file);
}

static void FreeAssets(void) {
  for (int i = 0; i < g_nassets; i++) {
    free((void *)g_assets[i].name);
    g_assets[i].name = NULL;
    BA_Clear(&g_assets[i].data);
  }
  g_nassets = 0;
}

static bool FileExists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  fclose(f);
  return true;
}

static bool LoadRomFile(const char *path) {
  size_t length = 0;
  uint8 *data = ReadWholeFile(path, &length);
  if (!data)
    return false;
  if ((length & 0xfffff) == 0x200) {
    length -= 0x200;
    memmove(data, data + 0x200, length);
  }
  char hex[41];
  Sha1Hex(data, length, hex);
  if (strcmp(hex, kUsRomSha1) != 0) {
    fprintf(stderr,
            "ROM SHA1 %s is not the US ALttP image (%s).\n"
            "Need: Legend of Zelda, The - A Link to the Past (USA)\n",
            hex, kUsRomSha1);
    free(data);
    return false;
  }
  g_rom = data;
  g_rom_size = length;
  return true;
}

#ifndef _WIN32
static bool RunPicker(const char *cmd, char *out, size_t out_sz) {
  FILE *f = popen(cmd, "r");
  if (!f)
    return false;
  if (!fgets(out, (int)out_sz, f)) {
    pclose(f);
    return false;
  }
  int st = pclose(f);
  if (!WIFEXITED(st) || WEXITSTATUS(st) != 0)
    return false;
  size_t n = strlen(out);
  while (n && (out[n - 1] == '\n' || out[n - 1] == '\r'))
    out[--n] = 0;
  return n > 0;
}
#endif

static bool PickRomSdlDrop(char *out, size_t out_sz) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    return false;
  SDL_Window *w = SDL_CreateWindow(
      "Drop a Zelda 3 ROM (USA .sfc / .smc)",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 560, 200, 0);
  if (!w) {
    SDL_Quit();
    return false;
  }
  SDL_Renderer *r = SDL_CreateRenderer(w, -1, 0);
  bool got = false;
  bool running = true;
  while (running) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE))
        running = false;
      else if (ev.type == SDL_DROPFILE && ev.drop.file) {
        snprintf(out, out_sz, "%s", ev.drop.file);
        SDL_free(ev.drop.file);
        got = true;
        running = false;
      }
    }
    if (r) {
      SDL_SetRenderDrawColor(r, 16, 16, 20, 255);
      SDL_RenderClear(r);
      SDL_RenderPresent(r);
    }
    SDL_Delay(16);
  }
  if (r)
    SDL_DestroyRenderer(r);
  SDL_DestroyWindow(w);
  SDL_Quit();
  return got;
}

static bool PickRomPath(char *out, size_t out_sz) {
#ifdef _WIN32
  OPENFILENAMEA ofn;
  memset(&ofn, 0, sizeof(ofn));
  out[0] = 0;
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = "SNES ROM (*.sfc;*.smc)\0*.sfc;*.smc\0All files\0*.*\0";
  ofn.lpstrFile = out;
  ofn.nMaxFile = (DWORD)out_sz;
  ofn.lpstrTitle = "Select A Link to the Past (USA) ROM";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  return GetOpenFileNameA(&ofn) != 0;
#else
#ifdef __APPLE__
  if (RunPicker(
          "osascript -e 'try' -e 'POSIX path of (choose file with prompt \"Select a Zelda 3 ROM\" of type {\"sfc\",\"smc\",\"SFC\",\"SMC\"})' -e 'end try'",
          out, out_sz))
    return true;
#endif
  if (RunPicker(
          "zenity --file-selection --title=\"Select A Link to the Past (USA) ROM\" --file-filter='SNES ROM | *.sfc *.smc' --file-filter='All files | *'",
          out, out_sz))
    return true;
  if (RunPicker("kdialog --getopenfilename . '*.sfc *.smc'", out, out_sz))
    return true;
  return PickRomSdlDrop(out, out_sz);
#endif
}

static void ShowExtractError(const char *msg) {
  fprintf(stderr, "Error: %s\n", msg);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Zelda3", msg, NULL);
}

void Extract_EnsureAssets(const char *rom_override) {
  if (FileExists(kAssetsPath))
    return;

  fprintf(stderr, "zelda3_assets.dat not found; extracting from a US ALttP ROM.\n");

  char path[4096];
  const char *rom = rom_override;
  if (!rom && FileExists("zelda3.sfc"))
    rom = "zelda3.sfc";
  else if (!rom && FileExists("zelda3.smc"))
    rom = "zelda3.smc";
  else if (!rom) {
    if (!PickRomPath(path, sizeof(path))) {
      ShowExtractError("No ROM selected. Pass --rom path/to/zelda3.sfc or place zelda3.sfc next to the game.");
      Die("No ROM selected");
    }
    rom = path;
  }

  fprintf(stderr, "Using ROM: %s\n", rom);
  if (!LoadRomFile(rom)) {
    ShowExtractError("That file is not the US A Link to the Past ROM.");
    Die("Unsupported ROM");
  }

  fprintf(stderr, "Extracting assets...\n");
  BuildAllAssets();
  WriteAssetsFile(kAssetsPathTmp);
  if (rename(kAssetsPathTmp, kAssetsPath) != 0)
    Die("Failed to replace zelda3_assets.dat");
  FreeAssets();
  free(g_rom);
  g_rom = NULL;
  fprintf(stderr, "Wrote %s\n", kAssetsPath);
}
