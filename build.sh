#!/usr/bin/env bash
# Builds the helpers the slurp wrapper uses (it falls back to plain slurp without them):
#  - ui-rects, the UI element detector: ~/.local/lib/xerahs-snap/ui-rects
#  - slurp + patches/slurp-*.patch (dashed border -D, click threshold, magnifier -Z):
#    ~/.local/lib/slurp-plus/slurp. Re-run after your distro updates slurp.
# Deps: gcc meson ninja git wayland wayland-protocols cairo libxkbcommon
# Usage: ./build.sh [slurp tag, default: pacman's slurp version, else v1.5.0]
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
installed=$(pacman -Q slurp 2>/dev/null | awk '{print $2}' | cut -d- -f1 || true)
VERSION="${1:-v${installed:-1.5.0}}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

gcc -O2 -Wall -o "$WORK/ui-rects" "$HERE/ui-rects/ui-rects.c"
install -Dm755 "$WORK/ui-rects" "$HOME/.local/lib/xerahs-snap/ui-rects"
echo "installed ui-rects -> ~/.local/lib/xerahs-snap/ui-rects"

git clone -q --depth 1 --branch "$VERSION" https://github.com/emersion/slurp "$WORK/slurp" 2>/dev/null
for p in dashed-border click-threshold loupe right-click-cancel; do
  git -C "$WORK/slurp" apply "$HERE/patches/slurp-$p.patch"
done
meson setup "$WORK/slurp/build" "$WORK/slurp" --buildtype=release >/dev/null
ninja -C "$WORK/slurp/build" slurp >/dev/null
install -Dm755 "$WORK/slurp/build/slurp" "$HOME/.local/lib/slurp-plus/slurp"
echo "installed slurp $VERSION + patches -> ~/.local/lib/slurp-plus/slurp"
