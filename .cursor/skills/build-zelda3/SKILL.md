---
name: build-zelda3
description: Extract ROM assets and compile zelda3. Use when building, running make, fixing gcc/C23 errors, extracting zelda3_assets.dat, or setting up the US ROM.
---

# Build zelda3

## Never commit

`zelda3.sfc`, `zelda3_assets.dat`, `tables/`, extracted `assets/` dumps. See [AGENTS.md](../../../AGENTS.md).

## Local build

```sh
python3 -m pip install -r requirements.txt
# zelda3.sfc in repo root, US, SHA256 66871d66be19ad2c34c927d6b14cd8eb6fc3181965b6e517cb361f7316009cfb
make -j$(nproc)
./zelda3
```

| Target | What |
| --- | --- |
| `make` / `make all` | Binary + `zelda3_assets.dat` (needs ROM) |
| `make zelda3` | Binary only. This is what CI runs. |
| `make clean` | Objects + generated assets |
| `python3 assets/restool.py --extract-from-rom` | Same extract as the Makefile |

Deps: SDL2 (`sdl2` on Arch), Python 3, Pillow, PyYAML.

## gcc 16 / C23

Makefile uses `-std=gnu17 -Werror`. Empty `()` is no args. Existing traps:

- `VWF_RenderSingle(int c)` in `src/messaging.h` (definition already took `c`)
- `ppu_init()` takes no args; callers must not pass `snes` / `NULL`

Do not drop `-Werror` or switch the whole tree to C23 to paper over prototypes.

## CI

`.github/workflows/build.yaml` has no ROM. `make zelda3` must stay ROM-free. If you add a source file under `src/*.c`, the wildcard picks it up; CI will compile it.
