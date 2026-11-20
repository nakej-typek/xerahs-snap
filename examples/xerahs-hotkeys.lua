-- Hyprland (Lua config, 0.56+): swallow the XerahS hotkeys so the focused app never sees them.
-- XerahS reads hotkeys from evdev without consuming them; when the selection layer grabs the
-- keyboard while S is still down, the app never gets the key release (a browser game keeps
-- "holding" S). XerahS still sees the keys. Keep this list in sync with your XerahS hotkeys.
-- Physical keycodes (layout independent): S 39, key left of 1 49, Print 107, G 42.
for _, combo in ipairs({ "ALT + code:39", "ALT + SHIFT + code:39", "ALT + code:49",
                         "ALT + code:107", "CTRL + code:107", "SHIFT + code:107", "CTRL + SHIFT + code:42" }) do
  hl.bind(combo, hl.dsp.no_op())
end

-- slurp's selection layer (with the magnifier) must vanish at once, or its fade-out ends up in
-- the screenshot grim takes right after; the freeze (hyprpicker) should not fade either.
hl.layer_rule({ name = "xerahs-capture-no-anim", match = { namespace = "^(selection|hyprpicker)$" }, no_anim = true })

-- A layer mapped under a still cursor gets no pointer enter, so a click right after Alt+S without
-- moving the mouse selected nothing: warp the cursor to where it already is once the layer opens.
local warpOnOpen = { selection = true, omasnap = true, ["omasnap-outside"] = true }
hl.on("layer.opened", function(layer)
  if not warpOnOpen[layer.namespace] then return end
  hl.timer(function()
    local pos = hl.get_cursor_pos()
    if pos then hl.dispatch(hl.dsp.cursor.move({ x = pos.x, y = pos.y })) end
  end, { timeout = 30, type = "oneshot" })
end)

-- Classic hyprland.conf equivalent (one line per hotkey):
--   bind = ALT, code:39, exec, true
--   layerrule = noanim, ^(selection|hyprpicker)$
