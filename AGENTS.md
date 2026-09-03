# AGENTS.md

Fork of [snesrev/zelda3](https://github.com/snesrev/zelda3): a C reimplementation of *A Link to the Past*. This repo is [jslay88/zelda3](https://github.com/jslay88/zelda3). Work against **this fork**. Do not open PRs on `snesrev/zelda3` unless the user says to.

Read `.cursor/rules/` on every session. For extract/build, use the `build-zelda3` skill. For 16:9 overworld edges, use the `widescreen-overworld` skill. `.cursorignore` keeps the ROM and extracted dumps out of the index.

## Hard rules

Never commit Nintendo IP or anything derived from the ROM:

- `zelda3.sfc`, `*.sfc`, `*.smc`
- `zelda3_assets.dat`
- extracted dumps: `assets/overworld/*.yaml`, `assets/dungeon/*.yaml`, `assets/img/`, `assets/sprites/*.png`, `assets/sound/`, `assets/dialogue*.txt`, `assets/font*.png`, `assets/linksprite.png`, `assets/hud_icons.png`, `tables/`
- `saves/`, `*.o`, the `zelda3` binary, `zelda3.user.ini`

Do not paste ROM bytes, map16 dumps, or ripped graphics into commits, PRs, or chat artifacts.

The US ROM is required **locally** to extract assets. SHA256:

```
66871d66be19ad2c34c927d6b14cd8eb6fc3181965b6e517cb361f7316009cfb
```

## Layout

| Path | What |
| --- | --- |
| `src/` | Game C. `Makefile` already wildcards `src/*.c`. |
| `snes/` | PPU / CPU / APU (LakeSnes-based). |
| `assets/` | Python extract + compile tools. Outputs are gitignored. |
| `src/ow_wide.c` | Neighbor-screen fill for 16:9 overworld side bars. |
| `zelda3.ini` | Checked-in defaults. `zelda3.user.ini` overrides it and is gitignored. |
| `.cursor/rules/` | Always-on and file-scoped agent rules. |
| `.cursor/skills/` | Task skills (`build-zelda3`, `widescreen-overworld`). |

## Build

```sh
python3 -m pip install -r requirements.txt   # Pillow, PyYAML
# Arch: pacman -S sdl2
# place zelda3.sfc in the repo root
make -j$(nproc)
./zelda3
```

`make` (default target) builds the binary **and** `zelda3_assets.dat` via `assets/restool.py --extract-from-rom`.

CI (`.github/workflows/build.yaml`) runs `make zelda3` only. There is no ROM on the runner, so do not make the default `make` target the CI command.

Compile as **gnu17** (`-std=gnu17` in the Makefile). C23 treats `()` as a no-arg prototype. `-Werror` is on. Do not "fix" that by deleting `-Werror`.

## Config that matters here

- `ExtendedAspectRatio = 16:9` (extra side pixels; `kPpuExtraLeftRight` is 96)
- `SeamlessOverworld = 1` (default true in `ParseConfigFile`). Draws neighbor map16 into the bars the PPU would otherwise leave black.

## Code conventions

- Match the file you are in. Most game C is CRLF. Do not convert line endings.
- Keep SNES names (`Overworld_LoadGFXAndScreenSize`, `BG2HOFS_copy2`, `overworld_tileattr`). Do not rename for taste.
- Types are `uint8` / `uint16` / `uint32` from `src/types.h`.
- Small, local diffs. No drive-by refactors, no whole-file reformats.
- New `src/*.c` files are picked up by the Makefile wildcard. You still need a header include from the call site.

## Git / PRs

- Atomic PRs. One concern per branch. Target `master` on **jslay88/zelda3**.
- `gh pr create --repo jslay88/zelda3 --base master --head <branch>` (a bare `gh pr create` will aim at snesrev).
- Short imperative commit subjects. No `Made with Cursor`, no `Co-authored-by` agent trailers.
- Confirm `git diff --name-only` has no ROM / extracted assets before you commit.

## Verify

- Touching C: `make -j$(nproc) zelda3` must succeed.
- Touching overworld / PPU side space: boot 16:9, walk screen edges, cut grass, cross a transition. Look for a center seam, lighting split, or path Y-shift.
- There is no unit test suite. Play the affected screen.
