#!/usr/bin/env bash
# Fail if copyrighted / ROM-derived paths are staged (default) or tracked (--tracked).
set -euo pipefail

mode=staged
if [[ "${1:-}" == "--tracked" ]]; then
  mode=tracked
elif [[ -n "${1:-}" ]]; then
  echo "usage: $0 [--tracked]" >&2
  exit 2
fi

if [[ "$mode" == tracked ]]; then
  mapfile -t files < <(git ls-files)
else
  mapfile -t files < <(git diff --cached --name-only)
fi

# Keep in sync with AGENTS.md and .cursor/rules/copyright-assets.mdc
# other/msu/encode_opus.py is a tool. Only the repo-root /msu/ dump is banned.
patterns=(
  '\.(sfc|smc|ips|bps)$'
  '(^|/)zelda3_assets\.dat$'
  '^tables/'
  '^msu/'
  '^sprites-gfx/'
  '^assets/overworld/.+\.ya?ml$'
  '^assets/dungeon/.+\.ya?ml$'
  '^assets/img/'
  '^assets/sprites/.+\.png$'
  '^assets/sound/'
  '^assets/dialogue.*\.txt$'
  '^assets/font.*\.png$'
  '^assets/linksprite\.png$'
  '^assets/hud_icons\.png$'
  '^assets/map32_to_map16\.txt$'
  '^assets/music_info\.ya?ml$'
  '^assets/sfx\.txt$'
  '^assets/sound_.+\.txt$'
  '^assets/generated_.+\.h$'
  '\.(pcm|brr|spc)$'
)

bad=()
for f in "${files[@]}"; do
  [[ -z "$f" ]] && continue
  for p in "${patterns[@]}"; do
    if echo "$f" | grep -Eiq "$p"; then
      bad+=("$f")
      break
    fi
  done
done

if ((${#bad[@]})); then
  echo "copyrighted / ROM-derived path staged or tracked. unstage and stop." >&2
  printf '%s\n' "${bad[@]}" >&2
  exit 1
fi

echo "OK: no copyrighted assets in ${mode} files"
