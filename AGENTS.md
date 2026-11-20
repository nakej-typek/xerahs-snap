# Setting up xerahs-snap (for a coding agent)

Goal: the user's XerahS region capture (`Alt+S` by default) works as described in README.md.
Work through the steps in order, check each one, and tell the user what you verified and what not.
Don't open windows on the user's screen to test unless they agree.

## 0. Check the prerequisites

- Hyprland session: `hyprctl version`. Other compositors: the wrappers fall back to plain slurp,
  so snapping to windows/layers, the freeze and the hotkey fix don't apply there.
- XerahS running, and how it is started: `systemctl --user list-units | grep -i xerahs`, or an
  autostart entry. You need this for step 3.
- Tools: `command -v grim slurp jq hyprpicker gcc meson ninja git`. Install the missing ones with
  the distro's package manager (Arch: `sudo pacman -S --needed grim slurp jq hyprpicker gcc meson ninja git wayland-protocols`).
  Ask the user to run commands that need sudo.

## 1. Install the wrappers and build the helpers

```sh
./install.sh
```

It copies `wrappers/{slurp,grim,omasnap}` to `~/.local/lib/xerahs-wrappers/` (it skips files
that are already symlinks, so it won't touch a dotfiles setup) and runs `build.sh`, which builds
`~/.local/lib/xerahs-snap/ui-rects` and the patched slurp in `~/.local/lib/slurp-plus/slurp`.
If the slurp build fails, everything except the dashed border and the magnifier still works.

## 2. Point XerahS at slurp

XerahS has to use slurp for region selection: the setting `LinuxRegionSelectorPreference` =
`Slurp` in the region capture workflow (`~/.config/xerahs/WorkflowsConfig-*.json`, or the
workflow's capture settings in the UI). For the editor after capture, `LinuxAnnotationEditor` =
`OmaSnap` in `ApplicationConfig.json` plus `AnnotateImage` in the workflow's after-capture
tasks (optional). Stop XerahS before editing its JSON files, it overwrites them on exit.

## 3. Give XerahS the wrappers

XerahS must find the wrappers first in `PATH` and use the omasnap wrapper. If it runs as a
systemd user unit, install `examples/xerahs-wrappers.conf` as a drop-in for that unit, then
`systemctl --user daemon-reload` and restart the unit. Otherwise set the same two variables
wherever XerahS is started. Check:

```sh
tr '\0' '\n' < /proc/$(pgrep -x xerahs)/environ | grep -E '^PATH=|XERAHS_OMASNAP_PATH'
```

The XerahS log (`~/.local/state/xerahs/Logs/`) should show `OmaSnap ... ready [<path to the wrapper>]`.

## 4. Swallow the hotkeys in Hyprland

Add `examples/xerahs-hotkeys.lua` to the Hyprland config (Lua config: paste or `require` it;
classic `hyprland.conf`: use the one-line form from the file's comment). Match the list to the
user's XerahS hotkeys. Run `hyprctl reload`, then `hyprctl -j binds` should list the new binds.
Don't enable XerahS' own `LinuxHyprlandKeybindings` at the same time.

## 5. Verify

- Wrapper logic without the GUI: replace slurp by a fake (`sed "s|^real=/usr/bin/slurp|real=/path/to/fake|"`
  into a temp copy) that prints its arguments and returns e.g. `500 300 2 9`. The output must
  be the window under that point, not a 2×9 box.
- `~/.local/lib/xerahs-snap/ui-rects < shot.ppm` on a `grim -s 1 -t ppm shot.ppm` lists boxes.
- Then ask the user to press the hotkey: frozen, dimmed screen, dashed border, magnifier;
  click on a window, on a button, drag across monitors; the editor opens on that monitor.

## Knobs

- `XERAHS_FREEZE=0`: no freeze. `XERAHS_SNAP_UI=0`: no UI element snapping (windows only).
- Colours and the dash length are in `STYLE=` at the top of `wrappers/slurp`.
- After a slurp package update, re-run `./build.sh`.

## Undo

Remove the drop-in (or the env vars) and the Hyprland binds, restart XerahS,
`rm -r ~/.local/lib/xerahs-wrappers ~/.local/lib/xerahs-snap ~/.local/lib/slurp-plus`.
