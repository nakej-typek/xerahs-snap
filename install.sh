#!/usr/bin/env bash
# Installs the wrappers into ~/.local/lib/xerahs-wrappers and builds the helpers.
# Existing symlinks there (e.g. managed by a dotfiles repo) are left alone.
# The systemd and Hyprland steps are in AGENTS.md.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
DEST="$HOME/.local/lib/xerahs-wrappers"
mkdir -p "$DEST"
for f in slurp grim omasnap; do
  if [[ -L $DEST/$f ]]; then
    echo "skip $DEST/$f (symlink, managed elsewhere)"
  else
    install -m755 "$HERE/wrappers/$f" "$DEST/$f"
    echo "installed $DEST/$f"
  fi
done
"$HERE/build.sh" "$@"
