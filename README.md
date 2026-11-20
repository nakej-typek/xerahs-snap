# xerahs-snap

ShareX-style region capture for [XerahS](https://github.com/ShareX/XerahS) on Hyprland.

- The screen freezes and dims on the hotkey.
- Hover snaps to windows, the bar, popups and UI elements inside windows (panels, buttons, tables).
- The selection has a dashed border, and a magnifier follows the cursor.
- Click = what's under the cursor, drag = free region (also across monitors). A wobbly click doesn't give you a 2×9 px screenshot.
- The OmaSnap editor opens on the monitor you selected on.
- Hotkeys don't leak into the focused app (no stuck keys in games).

It's a set of wrappers around `slurp`/`grim`/`omasnap` that XerahS calls, plus a small UI element detector and three slurp patches. Without the patched slurp you only lose the dashed border and the magnifier.

**Setup:** give [AGENTS.md](AGENTS.md) to your coding agent, or follow it by hand.

Needs: Hyprland, XerahS (Linux region selector = Slurp), `grim`, `slurp`, `jq`, `hyprpicker`; to build: `gcc meson ninja`.

MIT. Written with help from Claude Code.
